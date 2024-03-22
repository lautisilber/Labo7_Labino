from __future__ import annotations
import dataclasses
from typing import Generator, Optional, Union
import os

from logging_helper import logger as lh

from serial_manager import SerialManager
from balanzas import Balanzas
from balanzas import calibrate as balanzas_calibrate
from file_manager import FileManager
from smart_arrays import SmartArray
import smart_arrays.smart_array as sa
from dataclass_save import save_dataclass
from maintenance_circuit import Maintenance

from collections import deque

from time import sleep
from datetime import datetime

@dataclasses.dataclass(frozen=True)
class IntensityConfig:
    '''
    Class that defines a function that takes in a parameter between
    0 and n_steps and outputs a value between initial_value and final_value
    the resulting value will be initial value if the argument is smaller
    than n_steps_not_incrementing. Then it increments linearly until final_value
    '''
    initial_value: int
    final_value: int
    n_steps: int
    n_steps_not_incrementing: int

    def __post_init__(self) -> None:
        if self.final_value < self.initial_value:
            raise ValueError()
        if self.n_steps < self.n_steps_not_incrementing:
            raise ValueError()

    def __call__(self, arg: int) -> int:
        if arg < self.n_steps_not_incrementing: return self.initial_value
        elif arg >= self.n_steps: return self.final_value

        x_range = self.n_steps - self.n_steps_not_incrementing
        y_range = self.final_value - self.initial_value
        norm_arg = (arg - self.n_steps_not_incrementing) / x_range
        return norm_arg * y_range + self.initial_value


@dataclasses.dataclass(frozen=True)
class Position:
    '''Class for keeping track of each pot's position'''
    stepper: int
    servo: int
    water_time_cruve: IntensityConfig
    water_pwm_curve: IntensityConfig

    def __eq__(self, other: Position) -> bool:
        if not isinstance(other, Position): raise TypeError()
        return self.stepper == other.stepper and self.servo == other.servo

    def __lt__(self, other: Position) -> bool:
        if not isinstance(other, Position): raise TypeError()
        if self.stepper < other.stepper:
            return True
        elif self.servo < other.servo:
            return True
        return False
    
    def __le__(self, other: Position) -> bool:
        return self.__eq__(other) or self.__lt__(other)

@dataclasses.dataclass(frozen=True)
class SerialManagerInfo:
    port: str = '/dev/ttyACM0'
    baud_rate: int = 9600
    timeout: int = 5
    delay_s: int = .15

@dataclasses.dataclass(frozen=True)
class BalanzasInfo:
    save_file: str
    n_statistics: int = 50
    n_arduino: int = 10
    err_threshold: int = 500

@dataclasses.dataclass(frozen=False)
class StepperPos:
    save_file: str
    pos: int=0

@dataclasses.dataclass(frozen=True)
class SystemInfo:
    name: str

    n_balanzas: int
    positions: tuple[Position, ...]
    stepper_pos: StepperPos
    
    sm_info: SerialManagerInfo
    balanzas_info: BalanzasInfo

    grams_goals: tuple[float, ...]
    grams_threshold: float

    savedir: str='.'

    @property
    def data_savefile(self) -> str: # for the file manager
        return os.path.join(self.savedir, 'data_' + self.name + '.csv')
    @property
    def save_file(self) -> str:
        return os.path.join(self.savedir, self.name + '.json')
    def steps_to_move_for_position(self, end_position: Union[int, Position]) -> int:
        if isinstance(end_position, Position):
            end_position = end_position.stepper
        elif not isinstance(end_position, int):
            raise TypeError(f'end_position was not an int or a Position. It was {type(end_position)}')
        return end_position - self.stepper_pos.pos
    def save_stepper_pos(self):
        save_dataclass(self.stepper_pos)
    
    def __post_init__(self) -> None:
        if self.n_balanzas != len(self.grams_goals):
            raise IndexError('self.n_balanzas debe coincidir con el largo de self.grams_goals')

    @staticmethod
    def get_savefile_from_name(name: str, savedir: str='.') -> str:
        return os.path.join(savedir, name + '.json')

def slice_deque(d: deque, stop: int, start: int=0, step: int=1, length_check: bool=True) -> Generator:
    if length_check:
        stop = min(stop, len(d))
        start = min(start, stop)
    return (d[i] for i in range(start, stop, step))

class SystemsManager:
    def __init__(self, systems: tuple[SystemInfo, ...], maintenance: Maintenance) -> None:
        self.systems = tuple(systems)
        self.n_systems = len(self.systems)
        self.serial_managers = tuple(
            SerialManager(
                port=s.sm_info.port,
                baud_rate=s.sm_info.baud_rate,
                delay_s=s.sm_info.delay_s)
            for s in self.systems
        )
        self.balanzas = tuple(
            Balanzas(
                serial_manager=sm,
                n_balanzas=s.n_balanzas,
                n_statistics=s.balanzas_info.n_statistics,
                n_arduino=s.balanzas_info.n_arduino,
                err_threshold=s.balanzas_info.err_threshold,
                save_file=s.balanzas_info.save_file) 
            for s, sm in zip(self.systems, self.serial_managers)
        )
        self.file_managers = tuple(
            FileManager(s.n_balanzas, s.data_savefile)
            for s in self.systems
        )
        self.intensities_all = list(
            sa.zeros(s.n_balanzas, int)
            for s in self.systems
        )
        self.last_weights_all = list(
            sa.zeros(s.n_balanzas, int)
            for s in self.systems
        )

        # checks
        self.history_length = 30
        # accessed like self.weights_history[system_index][balanza_inedex][history_index]
        # the left side of the deque is the newest
        self.weights_history: tuple[tuple[deque[float],...],...] = tuple(tuple(deque(maxlen=self.history_length) for _ in range(s.n_balanzas)) for s in self.systems)
        self.watering_history: tuple[tuple[deque[bool],...],...] = tuple(tuple(deque(maxlen=self.history_length) for _ in range(s.n_balanzas)) for s in self.systems)
        self.intensities_history: tuple[tuple[deque[int],...],...] = tuple(tuple(deque(maxlen=self.history_length) for _ in range(s.n_balanzas)) for s in self.systems)
        self.timing_history: tuple[deque[datetime],...] = tuple(deque(maxlen=self.history_length) for _ in range(self.n_systems)) # para esto no necesito una sublista para cada balanza, dado que las balanzas se miden en simultaneo
        self.failed_checks_history: tuple[tuple[deque[bool],...],...] = tuple(tuple(deque(maxlen=self.history_length) for _ in range(s.n_balanzas)) for s in self.systems)
        # accessed like self.weights_history[system_index][balanza_inedex]
        self.inhabilitated_balanzas: tuple[SmartArray,...] = tuple(sa.zeros(s.n_balanzas, bool) for s in self.systems)

        # flags
        self.halt_flag = False
        self.maintenance = maintenance
        self.maintenance.begin_maintenance(lambda: self.halt(True))
        self.maintenance.end_maintenance(lambda: self.halt(False))

    def halt(self, state: bool) -> None:
        self.halt_flag = state

    def check_halt(self) -> bool:
        if self.halt_flag:
            self.maintenance.led_pulse()
            return True
        else:
            self.maintenance.led_on()
            return False

    def begin_single(self, index: int) -> None:
        if index < 0 or index >= len(self.systems):
            raise IndexError(f'{index} < 0 or {index} >= {len(self.systems)}')
        
        self.balanzas[index].load()

        system = self.systems[index]
        sm = self.serial_managers[index]
        sm.open()
        sleep(1)

        # check port ok
        while not sm.cmd_ok():
            sleep(1)

        # detach stepper
        sm.cmd_servo_attach(False)

        # check that the number of balanzas matches
        n_balanzas = None
        while n_balanzas is None:
            n_balanzas = sm.cmd_hx_n()
            sleep(1)
        if not self.systems[index].n_balanzas == n_balanzas:
            raise ValueError('La cantidad de balanzas reportada y la esperada no son iguales')
    
    def begin(self) -> None:
        for b in self.balanzas:
            b.load()
        for s, sm in zip(self.systems, self.serial_managers):
            # open port
            sm.open()
            sleep(1)

            # check port ok
            while not sm.cmd_ok():
                sleep(1)

            # detach stepper
            sm.cmd_servo_attach(False)

            # check that the number of balanzas matches
            n_balanzas = None
            while n_balanzas is None:
                n_balanzas = sm.cmd_hx_n()
                sleep(1)
            if not s.n_balanzas == n_balanzas:
                raise ValueError('La cantidad de balanzas reportada y la esperada no son iguales')
            
    def show_all_positions(self, index: int, wait_for_user_input: bool=False) -> None:
        if index < 0 or index > len(self.systems):
            raise IndexError()
        system = self.systems[index]
        sm = self.serial_managers[index]
        for i, position in enumerate(system.positions):
            sm.cmd_servo(90)
            sm.cmd_stepper(system.steps_to_move_for_position(position), detach=True)
            system.stepper_pos.pos = position.stepper
            # save system to save stepper state
            system.save_stepper_pos()
            sm.cmd_servo(position.servo)
            if wait_for_user_input:
                input(f'Posicion {i}. Presiona cualquier tecla para continuar...')
            else:
                sleep(5)
        sm.cmd_servo_attach(False)

    def go_home(self, index: int, wait_for_user_input: bool) -> None:
        if index < 0 or index > len(self.systems):
            raise IndexError()
        system = self.systems[index]
        sm = self.serial_managers[index]
        if wait_for_user_input:
            input(f'Stepper current saved position is {system.stepper_pos.pos}. If correct press any button...')
            print('going to home position')
        sm.cmd_servo(90)
        sm.cmd_stepper(system.steps_to_move_for_position(0), detach=True)
        system.stepper_pos.pos = 0
        system.save_stepper_pos()
        sm.cmd_servo_attach(False)

    def calibrate_system(self, index: int, n: int=200) -> None:
        if index < 0 or index >= len(self.systems):
            raise IndexError()
        self.serial_managers[index].cmd_stepper_attach(False)
        self.serial_managers[index].cmd_servo_attach(False)
        balanzas = self.balanzas[index]
        n_balanzas = self.systems[index].n_balanzas
        balanzas_calibrate(
            balanzas=balanzas,
            n_balanzas=n_balanzas,
            n=n,
            offset_temp_file=f'.tmp_balanzas_offset_system_{index}.json'
        )

    def show_system_weights(self, index: int, n: Optional[int]=None) -> None:
        print(self.balanzas[index].read_stats(n))

    def water_test(self, system_index: int, balanza_index: int, intensity: int=0) -> None:
        if system_index < 0 or system_index >= len(self.systems):
            raise IndexError()
        system = self.systems[system_index]
        if balanza_index < 0 or balanza_index >= system.n_balanzas:
            raise IndexError()
        sm = self.serial_managers[system_index]
        position = system.positions[balanza_index]
        sm.cmd_servo(90)
        sm.cmd_stepper(system.steps_to_move_for_position(position), detach=True)
        system.stepper_pos.pos = position.stepper
        # save system to save stepper state
        system.save_stepper_pos()
        sm.cmd_servo(position.servo)

        tiempo_ms = position.water_time_cruve(intensity)
        pwm = position.water_pwm_curve(intensity)
        sm.cmd_pump(tiempo_ms, pwm)

        sm.cmd_servo_attach(False)

    @staticmethod
    def water(position: Position, sm: SerialManager, intensity: int, system: SystemInfo) -> None:
        sm.cmd_servo(90)
        sm.cmd_stepper(system.steps_to_move_for_position(position), detach=True)
        system.stepper_pos.pos = position.stepper
        # save system to save stepper state
        system.save_stepper_pos()
        sleep(.5)
        sm.cmd_servo(position.servo)

        tiempo_ms = position.water_time_cruve(intensity)
        pwm = position.water_pwm_curve(intensity)
        sm.cmd_pump(tiempo_ms, pwm)

        sm.cmd_servo_attach(False)

    def _check_all_right(self, system_index: int, balanza_index: int) -> bool:
        if system_index < 0 or system_index >= self.n_systems:
            raise IndexError()
        system = self.systems[system_index]
        if balanza_index < 0 or balanza_index >= system.n_balanzas:
            raise IndexError()
        weight_history = self.weights_history[system_index][balanza_index]
        watering_history = self.watering_history[system_index][balanza_index]
        intensities_history = self.intensities_history[system_index][balanza_index]
        timing_history = self.timing_history[system_index]
        failed_checks_history = self.failed_checks_history[system_index]

        # si el valor es negativo!
        if weight_history[0] < 0:
            lh.critical((f'check before watering: sistema {system_index}, balanza {balanza_index} '
                        f'-> No paso el chequeo para regar. El peso es negativo ({weight_history[0]})'))
            return False
        
        recent_history = self.history_length - 15
        if len(watering_history) >= recent_history:
            # si viene regando a full hace mucho (no contandi fallos del chequeo)
            # no se puede indexar un deque con un slice: deaque[1:5] no es posible
            c1 = tuple(w for w, f in zip(slice_deque(watering_history, recent_history), slice_deque(failed_checks_history, recent_history)) if not f)
            c2 = tuple(i > 10 for i, f in zip(slice_deque(intensities_history, recent_history), slice_deque(failed_checks_history, recent_history)) if not f)
            if (all(c1) and len(c1) > 0) and (all(c2) and len(c2) > 0):
                lh.critical((f'check before watering: sistema {system_index}, balanza {balanza_index} '
                            '-> No paso el chequeo para regar. Esta regando demasiado intenso demasiadas '
                            f'veces (watering_history={watering_history}, intensities_history={intensities_history}, '
                            f'weight_history={weight_history})'))
                return False
        
        # si vienen muchos valores negativos -> INHABILITAR
        if sum(w < 0 for w in weight_history) > 5:
            lh.critical((f'check before watering: sistema {system_index}, balanza {balanza_index} '
                        f'-> No paso el chequeo para regar. tuvo mas de 5 pesos negativos ({weight_history}). '
                        'Inhabilitando balanza hasta intervencion manual'))
            self.inhabilitated_balanzas[system_index][balanza_index] = True
            return False
        
        # TODO: seguir completando casos
        return True

    def tick_single(self, index: int) -> Optional[tuple[SmartArray, SmartArray]]:
        '''
            if return is None, the system was halted.
            else, it returns means and macetas_to_water
        '''
        if self.check_halt():
            return None

        system = self.systems[index]
        serial_manager = self.serial_managers[index]
        balanzas = self.balanzas[index]
        file_manager = self.file_managers[index]
        grams_goals = SmartArray(system.grams_goals, float)
        intensities = self.intensities_all[index]
        grams_threshold = system.grams_threshold

        # leer datos
        res = None
        while res is None:
            res = balanzas.read_stats()
            if res is None:
                sleep(1)
                lh.warning(f'Sistema {index}: No se pudo leer las balanzas. Volviendo a intentar...')
        vals, n_filtered, n_unsuccessful = res
        if len(vals) != system.n_balanzas:
            lh.warning(f'Sistema {index}: Al leer se obtuvo una lista de largo {len(vals)} cuando hay {system.n_balanzas} balanzas')

        means = vals.values()
        stdevs = vals.errors()

        macetas_to_water = means < (grams_goals - grams_threshold)

        # agregar datos de mediciones para chequear que todo esta en orden
        self.timing_history[index].appendleft(datetime.now().strftime('%Y-%m-%d_%H-%M-%S'))
        for i in range(system.n_balanzas):
            self.weights_history[index][i].appendleft(means[i])
            self.watering_history[index][i].appendleft(macetas_to_water[i])
            self.intensities_history[index][i].appendleft(intensities[i])
            self.failed_checks_history[index][i].appendleft(self.inhabilitated_balanzas[index][i])
        lh.debug(f'Datos de mediciones - sistema {index}: pesos={vals}, a_regar={macetas_to_water}, intensidades={intensities}')

        if self.check_halt():
            return None

        for i, (w, intensity, position) in enumerate(zip(macetas_to_water, intensities, system.positions)):
            if i in (0,1): continue
            if self.inhabilitated_balanzas[index][i]: continue
            if w:
                if self._check_all_right(index, i):
                    SystemsManager.water(
                        position=position,
                        sm=serial_manager,
                        intensity=intensity,
                        system=system
                    )
                    lh.info(f'Tick: Watering {i}, starting with weight {means[i]} +/- {stdevs[i]}, goal of {grams_goals[i]} and threashold of {grams_threshold}')
                else:
                    self.failed_checks_history[index][i][-1] = True
        
        res = serial_manager.cmd_dht()
        if res is not None:
            hum, temp = res
        else:
            hum = None
            temp = None

        file_manager.add_entry(
            means, stdevs, macetas_to_water, n_filtered, n_unsuccessful,
            grams_goals, grams_threshold, hum, temp
        )

        return means, macetas_to_water
 
    def loop(self):
        first = True
        min_weight_diff = 5

        while True:
            if not self.check_halt():
                for i in range(self.n_systems):
                    res = self.tick_single(i)
                    if res is None:
                        break
                    weights, watered_last_tick = res

                    if not first:
                        first = False

                        weight_diff = abs(weights - self.last_weights_all[i])
                        self.intensities_all[i] = (watered_last_tick * (weight_diff < min_weight_diff)).int()

                    self.last_weights_all[i] = weights
            else:
                sleep(.1)

