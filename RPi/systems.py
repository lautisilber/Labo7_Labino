from __future__ import annotations
from serial_manager import SerialManager
from balanzas import Balanzas
from balanzas import calibrate as balanzas_calibrate
from file_manager import FileManager
from camera_controller import CameraController
from logging_helper import logger as lh
from smart_arrays import SmartArrayFloat, SmartArrayInt, SmartArrayBool, SmartArray
from smart_arrays.utils import loadtxt as smart_loadtxt
from smart_arrays.utils import savetxt as smart_savetxt
from smart_arrays import stats


import os
import dataclasses
from collections import deque
from datetime import datetime, timedelta
import json
import pickle
from typing import Any, Literal, Optional, Union
from time import sleep


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
        if arg < self.n_steps_not_incrementing:
            return self.initial_value
        elif arg >= self.n_steps:
            return self.final_value

        x_range = self.n_steps - self.n_steps_not_incrementing
        y_range = self.final_value - self.initial_value
        norm_arg = (arg - self.n_steps_not_incrementing) / x_range
        return int(norm_arg * y_range + self.initial_value)


@dataclasses.dataclass(frozen=True)
class Position:
    '''Class for keeping track of each pot's position'''
    stepper: int
    servo: int
    water_time_cruve: IntensityConfig
    water_pwm_curve: IntensityConfig

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Position):
            raise TypeError()
        return self.stepper == other.stepper and self.servo == other.servo

    def __lt__(self, other: Position) -> bool:
        if not isinstance(other, Position): # type: ignore
            raise TypeError()
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
    delay_s: float = .15
    parity: Literal['N', 'E', 'O'] = 'E'


@dataclasses.dataclass(frozen=True)
class BalanzasInfo:
    n_statistics: int = 50
    n_arduino: int = 10
    err_threshold: int = 500


@dataclasses.dataclass(frozen=True)
class CameraControllerInfo:
    n_files: int = 50
    resolution: tuple[int, int] = (640, 480)
    framerate: int = 20


@dataclasses.dataclass(frozen=True)
class WateringScheduleStep:
    '''
    a watering schedule step is a weight goal with a time that that weight should be held.
    if weigh_threshold is rgeater than 0
    if max_weight_difference is None, nothing special happens. if it is a positive number,
    when a step is done, the weight goal has to be self.weight +- self.max_weight_difference
    (that is to prevent a case where step 1 is very heavy, and step 2 very light. then step 2 will be
    immediately completed after step 1)
    '''
    weight: float
    time: timedelta = timedelta()
    weight_threshold: float = 0
    max_weight_difference: Optional[float] = None

    @property
    def has_time(self) -> bool:
        return self.time > timedelta()

    def __post_init__(self):
        if self.weight_threshold < 0:
            raise ValueError(f'weight_threshold should be 0 or positive. it is {
                             self.weight_threshold}')
        if self.max_weight_difference is not None:
            if self.max_weight_difference < 0:
                raise ValueError(f'max_weight_difference should be 0 or positive. it is {
                                 self.max_weight_difference}')
            if self.weight_threshold - 1 >= self.max_weight_difference:
                raise ValueError(
                    'weight_threshold (- 1 for errors) cannot be greater than max_weight_difference, otherwise goal will never be reached')
        if not isinstance(self.time, timedelta):  # type: ignore
            raise TypeError(f'time shoud be a timedelta. Instead it is {
                            type(self.time)}')


class WateringSchedule:
    def __init__(self, steps: tuple[WateringScheduleStep, ...], cyclic: bool) -> None:
        self.steps = steps
        self.cyclic = cyclic
        self.n_steps = len(self.steps)
        self._current_step: int = 0
        self._last_step_init_time: datetime = datetime.now()
        # if _got_to_weight_goal is false, the balanza has not yet reached the weight goal for this step.
        # if true, the goal for this step has been reached and we are waiting on the time corresponding to this step.
        self._got_to_weight_goal: bool = False
        # guarda el peso obtenido cuando se completo el anterior paso
        self._last_steps_weight: Optional[float] = None
        # if self.cyclic is True, this variable tracks the number of complete cycles taken. If False, it is set to 1 once
        # the schedule has reached its final step's goal
        self.n_cycles: int = 0

    @property
    def current_step(self) -> int:
        return self._current_step

    def in_goal(self, curr_weight: float) -> bool:
        '''check if curr_weight is in goal for this step (!= should water)'''
        step = self.steps[self._current_step]
        if step.max_weight_difference is not None:
            return step.weight - step.max_weight_difference <= curr_weight <= step.weight + step.max_weight_difference
        else:
            # como no hay un max_weight_difference, se estara en un goal dependiendo de si el paso anterior tenia un peso objetivo mayor o menor a este
            if self._last_steps_weight is None:
                return False  # si no tenemos peso anterior, no podemos hacer nada porque estamos en el primer tick. Esperar hasta que haya un historial
            if (not self.cyclic and self.n_cycles >= 1):  # if reached last step and not cyclic
                # en este caso siempre hay que entender que el objetivo se llega si el peso es mayor al objetivo, dado que queremos mantener la
                # maceta en este peso. Esto debe ser asi porque no ocurre el unico ccaso en que chequeamos que el peso sea menor, que ocurre cuando
                # partimos de un peso mayor y debemos esperar hasta que se seque.
                return curr_weight >= step.weight - step.weight_threshold
            elif step.weight > self._last_steps_weight:
                return curr_weight >= step.weight - step.weight_threshold
            elif step.weight < self._last_steps_weight:
                return curr_weight <= step.weight - step.weight_threshold
            return True

    def _next_step(self, curr_weight: float) -> None:
        self._current_step += 1
        if self._current_step >= self.n_steps:
            if self.cyclic:
                self._current_step = 0
                self.n_cycles += 1
            else:
                self._current_step = self.n_steps - 1
                self.n_cycles = 1
        self._got_to_weight_goal = False
        self._last_steps_weight = curr_weight

    def should_water(self, curr_weight: float) -> bool:
        if self.in_goal(curr_weight):
            return False
        step = self.steps[self._current_step]
        if step.weight > curr_weight:
            return True
        else:
            return False

    def update(self, curr_weight: float) -> None:
        '''
        this also updates the current step!

        i'll explain this mess
        if the goal has not been reached for this step, set the flag (_got_to_weight_goal) to true and record the time when it happened
        if the goal has been reached for this step:
            if step time is 0, go to next step and resample step
            if step time is > 0, check if the difference between now and the datetime flag (_last_step_init_time) is greater than step.time:
                if is, go to next step and resample step
        either way (if step was increased, the step object has been resampled), water if necessary following the step's goal
        '''
        step = self.steps[self._current_step]
        if not self._got_to_weight_goal:
            if self.in_goal(curr_weight):
                self._got_to_weight_goal = True
                self._last_step_init_time = datetime.now()
                self.update(curr_weight)
        else:
            if not step.has_time:
                self._next_step(curr_weight)
            else:
                if datetime.now() - self._last_step_init_time >= step.time:
                    self._next_step(curr_weight)
        if self._last_steps_weight is None:
            self._last_steps_weight = curr_weight

    @property
    def current_goal(self) -> float:
        '''returns the weight goal for the current step'''
        step = self.steps[self._current_step]
        return step.weight


class System:
    '''
    A systems stores all parts that belong to a single balanzas array (BA). That includes the following:
    - list(Positions): a list of len n_balanzas that contain the Positions of each balanza. Positions have info about servo and stepper position
                       as well as info about how to operate the pump for this balanza in particular
    - SerialManager: to connect to that BAs Arduino
    - Balanzas: to manage the balanzas of that Arduino
    - FileManager: to manage the data logging file
    - CameraControler: This can be None. If it is a CameraController, it will be used to record the system
    - list(WateringSchedules): a list of len n_balanzas that contain the WateringSchedules for each balanza. WateringSchedules have info about whether
                             a balanza should be watered given its weight. WateringSchedules can have mutliple steps which are cycled through automatically
    - n_balanzas: int -> the number of balanzas connected to the Arduino
    - save_dir: str -> a string denoting a directory. All saved files related to this system will be saved inside this directory. Has to be unique
    - stepper_pos: int -> since the stepper position is relative, an absolute position must be recorded. This variable is carefully managed to
                          try to have it matching to the real life counterpart, even when the program fails

    - intensities: SmartArray[int] -> This is an array of length n_balanzas and keeps track of the watering intensity of each balanza. This
                                      variable is stored on disk using pickle to preserve continuity between sessions
    - last_weights: SmartArray[float] -> This is an array of length n_balanzas and keeps track of the last weights sensed for each balanza. This
                                         variable is stored on disk using pickle to preserve continuity between sessions

    - histories: The "histories" of different variables are also stored for posterior analysis (can be used to determin wether a balanza is being
                 managed badly). These variables are also stored on disk using pickle to preserve continuity between sessions
        - the history_length is an int that indicates the length the histories will have. That is how many records can they contain before
          the oldest ones are deleted

    - name is the name of the system (not that relevant). Hast to be unique

    - watering related variables:
        - watering_min_diff_grams: if a maceta was watered, but is weight difference is lower than watering_min_diff_grams, it is considered that no
          water reached the maceta
        - watering_max_diff_grams: if a maceta is watered and its weight difference is greater than watering_max_diff_grams, its intensity is decreased
        - watering_max_unchanged_times: if a maceta was supposed to be watered, but it's considered to not have been watered (see watering_diff_min_grams)
          for more than watering_max_unchanged_times times consecutively, its intensity is increased by one
        - watering_intensity_lowering_rate: if a macetas intensity is lowered (see watering_max_diff_grams), its lowered by 'watering_intensity_lowering_rate'
          amount
    '''
    names: list[str] = list()

    def __init__(self, positions: tuple[Position, ...], sm_info: SerialManagerInfo, balanzas_info: BalanzasInfo, watering_schedules: tuple[WateringSchedule, ...],
                 cc_info: Optional[CameraControllerInfo], n_balanzas: int, name: str, save_dir: str, history_length: int = 30, watering_min_diff_grams: float = 1,
                 watering_max_diff_grams: float = 4, watering_max_unchanged_times: int = 2, watering_intensity_lowering_rate: int = 2,
                 use_tqdm: bool = True) -> None:
        if name in System.names:
            raise ValueError(
                f'The name "{name}" as already been used. Choose a different one')
        self.n_balanzas = n_balanzas
        self.save_dir = save_dir
        self.name = name

        # visual
        self.use_tqdm = use_tqdm

        if not len(positions) == self.n_balanzas:
            raise ValueError(
                f'System {self.name}: n_balanzas != len(positions)')
        if not len(watering_schedules) == self.n_balanzas:
            raise ValueError(
                f'System {self.name}: n_balanzas != len(watering_schedules)')
        self.positions = positions
        self.serial_manager = SerialManager(
            port=sm_info.port,
            baud_rate=sm_info.baud_rate,
            parity=sm_info.parity,
            timeout=sm_info.timeout,
            delay_s=sm_info.delay_s
        )
        self.balanzas = Balanzas(
            serial_manager=self.serial_manager,
            n_balanzas=self.n_balanzas,
            n_statistics=balanzas_info.n_statistics,
            n_arduino=balanzas_info.n_arduino,
            err_threshold=balanzas_info.err_threshold,
            save_file=os.path.join(
                self.save_dir, 'config', f'balanzas_{self.name}.json'),
            use_tqdm=self.use_tqdm
        )
        self.file_manager = FileManager(
            n_balanzas=self.n_balanzas,
            save_file=os.path.join(self.save_dir, 'data',
                                   f'data_{self.name}.csv')
        )
        self.watering_schedules = watering_schedules
        self.camera_controller = None if cc_info is None else CameraController(
            save_dir=os.path.join(self.save_dir, 'media'),
            n_files=cc_info.n_files,
            resolution=cc_info.resolution,
            framerate=cc_info.framerate
        )

        self.stepper_pos: int = 0

        self.intensities = SmartArrayInt.zeros(self.n_balanzas)
        self.last_weights: Optional[SmartArrayFloat] = None
        # this name is awful but it is intended for keeping track of how many consecutive times a balanza was
        # meant to be watered, but its weight change was lower than a threshold value
        self.intensities_unchanged_times = SmartArrayInt.zeros(self.n_balanzas)

        if watering_min_diff_grams >= watering_max_diff_grams:
            raise ValueError(f'System {self.name}. watering_min_diff_grams has to be lower than watering_max_diff_grams. ({
                             watering_min_diff_grams}, {watering_max_diff_grams})')
        self.watering_min_diff_grams = watering_min_diff_grams
        self.watering_max_diff_grams = watering_max_diff_grams
        self.watering_max_unchanged_times = watering_max_unchanged_times
        self.watering_intensity_lowering_rate = watering_intensity_lowering_rate

        self.inhabilitated_balanzas = SmartArrayBool.zeros(self.n_balanzas)

        self.history_length = history_length
        self.history_weights: tuple[deque[float], ...] = tuple(
            deque(maxlen=self.history_length) for _ in range(self.n_balanzas))
        self.history_watering: tuple[deque[bool], ...] = tuple(
            deque(maxlen=self.history_length) for _ in range(self.n_balanzas))
        self.history_intensities: tuple[deque[int], ...] = tuple(
            deque(maxlen=self.history_length) for _ in range(self.n_balanzas))
        self.history_datetimes: deque[datetime] = deque(
            maxlen=self.history_length)
        self.history_failed_checks: tuple[deque[bool], ...] = tuple(
            deque(maxlen=self.history_length) for _ in range(self.n_balanzas))
        self.history_inhabilitated_balanzas: deque[SmartArrayBool] = deque(
            maxlen=self.history_length)

    @property
    def save_file_stepper(self) -> str:
        return os.path.join(self.save_dir, 'config', f'stepper_{self.name}.json')

    @property
    def save_file_history(self) -> str:
        return os.path.join(self.save_dir, 'config', f'history_{self.name}.pkl')

    @property
    def save_file_inhabilitated(self) -> str:
        return os.path.join(self.save_dir, 'config', f'inhabilitated_{self.name}.txt')

    @property
    def save_file_intensities(self) -> str:
        return os.path.join(self.save_dir, 'config', f'intensities_{self.name}.txt')

    def _create_save_dir(self, dirname: Optional[str] = None) -> None:
        if dirname is None:
            dirname = self.save_dir
        if not os.path.isdir(dirname):
            os.makedirs(dirname)

    def save_stepper(self) -> None:
        fname = self.save_file_stepper
        self._create_save_dir(os.path.dirname(fname))
        obj = {'stepper': self.stepper_pos}
        with open(fname, 'w') as f:
            json.dump(obj, f)

    def load_stepper(self) -> bool:
        fname = self.save_file_stepper
        if not os.path.isfile(fname):
            return False
        with open(fname, 'r') as f:
            obj = json.load(f)
        if not 'stepper' in obj:
            raise IndexError(
                f'System {self.name}: Stepper save has no stepper key')
        if not isinstance(obj['stepper'], int):
            raise ValueError(
                f'System {self.name}: Stepper save stepper key is not an int')
        self.stepper_pos = obj['stepper']
        return True

    def save_history(self) -> None:
        fname = self.save_file_history
        self._create_save_dir(os.path.dirname(fname))
        objs = (
            self.history_weights,
            self.history_watering,
            self.history_intensities,
            self.history_datetimes,
            self.history_failed_checks,
            self.history_inhabilitated_balanzas
        )
        with open(fname, 'wb') as f:
            pickle.dump(objs, f)

    def load_history(self) -> tuple[bool, ...]:
        s = SmartArrayBool.zeros(self.n_balanzas)
        fname = self.save_file_history
        if not os.path.isdir(fname):
            return tuple(s)
        with open(fname, 'rb') as f:
            objs: Union[tuple[
                Union[tuple[deque[float], ...], Any], # history weights
                Union[tuple[deque[bool], ...], Any], # history watering
                Union[tuple[deque[int], ...], Any], # history intensities
                Union[deque[datetime], Any], # history datetimes
                Union[tuple[deque[bool], ...], Any], # history failed checks
                Union[deque[SmartArrayBool], Any] # history inhabilitated balanzas
            ], Any] = pickle.load(f)
        if not isinstance(objs, tuple):
            lh.warning(f'System {self.name}: Bad histories pickle')
            return tuple(s)
        if not len(objs) == 6:
            lh.warning(f'System {self.name}: Bad histories pickle')
            return tuple(s)
        # weights
        history_weights = objs[0]
        if all(isinstance(e, deque) for e in history_weights):
            if not all(all(isinstance(de, float) for de in te) for te in history_weights):
                s[0] = True
        if s[0]:
            history_weights = tuple(
                deque(e, maxlen=self.history_length) for e in history_weights)
            self.history_weights: tuple[deque[float], ...] = history_weights
        else:
            lh.warning(f'System {self.name}: Bad histories pickle')
        # watering
        history_watering = objs[1]
        if all(isinstance(e, deque) for e in history_watering):
            if not all(all(isinstance(de, float) for de in te) for te in history_watering):
                s[1] = True
        if s[1]:
            history_watering = tuple(
                deque(e, maxlen=self.history_length) for e in history_watering)
            self.history_watering: tuple[deque[bool], ...] = history_watering
        else:
            lh.warning(f'System {self.name}: Bad histories pickle')
        # intensities
        history_intensities = objs[2]
        if all(isinstance(e, deque) for e in history_intensities):
            if all(all(isinstance(de, int) for de in te) for te in history_intensities):
                s[2] = True
        if s[2]:
            history_intensities = tuple(
                deque(e, maxlen=self.history_length) for e in history_intensities)
            self.history_intensities: tuple[deque[int], ...] = history_intensities
        else:
            lh.warning(f'System {self.name}: Bad histories pickle')
        # datetimes
        history_datetimes = objs[3]
        if isinstance(history_datetimes, deque):
            if all(isinstance(e, datetime) for e in history_datetimes): # type: ignore
                s[3] = True
        if s[3]:
            history_datetimes = deque(
                history_datetimes, maxlen=self.history_length)
            self.history_datetimes: deque[datetime] = history_datetimes
        else:
            lh.warning(f'System {self.name}: Bad histories pickle')
        # failed_checks
        history_failed_checks = objs[4]
        if all(isinstance(e, deque) for e in history_failed_checks):
            if all(all(isinstance(de, bool) for de in te) for te in history_failed_checks):
                s[4] = True
        if s[4]:
            history_failed_checks = tuple(
                deque(e, maxlen=self.history_length) for e in history_failed_checks)
            self.history_failed_checks: tuple[deque[bool], ...] = history_failed_checks
        else:
            lh.warning(f'System {self.name}: Bad histories pickle')
        # inhabilitated_balanzas
        history_inhabilitated_balanzas = objs[5]
        if isinstance(history_inhabilitated_balanzas, deque):
            if all(isinstance(e, SmartArrayBool) for e in history_inhabilitated_balanzas): # type: ignore
                s[5] = True
        if s[5]:
            history_inhabilitated_balanzas = deque(
                history_inhabilitated_balanzas, maxlen=self.history_length)
            self.history_inhabilitated_balanzas: deque[SmartArrayBool] = history_inhabilitated_balanzas
        else:
            lh.warning(f'System {self.name}: Bad histories pickle')

        return tuple(s)

    def save_inhabilitated_balanzas(self) -> None:
        fname = self.save_file_inhabilitated
        self._create_save_dir(os.path.dirname(fname))
        smart_savetxt(self.inhabilitated_balanzas, fname)

    def load_inhabilitated_balanzas(self) -> bool:
        fname = self.save_file_inhabilitated
        if not os.path.isfile(fname):
            return False
        try:
            inhabilitated_balanzas = smart_loadtxt(fname)
        except:
            raise ValueError(
                f'System {self.name}: loaded bad inhabilitated_balanzas. Bad file')
        if not isinstance(inhabilitated_balanzas, SmartArrayBool):
            raise ValueError(
                f'System {self.name}: loaded bad inhabilitated_balanzas. Bas typing')
        if len(inhabilitated_balanzas) != self.n_balanzas:
            raise ValueError(
                f'System {self.name}: loaded bad inhabilitated_balanzas. Bad size')
        self.inhabilitated_balanzas = inhabilitated_balanzas
        return True

    def save_intensities(self) -> None:
        fname = self.save_file_intensities
        self._create_save_dir(os.path.dirname(fname))
        smart_savetxt(self.intensities, fname)

    def load_intensities(self) -> bool:
        fname = self.save_file_intensities
        if not os.path.isfile(fname):
            return False
        try:
            intensities = smart_loadtxt(fname)
        except:
            raise ValueError(
                f'System {self.name}: loaded bad intensities. Bad file')
        if not isinstance(intensities, SmartArrayInt):
            raise ValueError(
                f'System {self.name}: loaded bad intensities. Bas typing')
        if len(intensities) != self.n_balanzas:
            raise ValueError(
                f'System {self.name}: loaded bad intensities. Bas size')
        self.intensities = intensities
        return True

    def begin(self) -> None:
        self.balanzas.load()
        self.load_stepper()
        self.load_history()
        self.load_inhabilitated_balanzas()
        self.load_intensities()

        self.serial_manager.open()
        sleep(.5)
        # check port ok
        while not self.serial_manager.cmd_ok():
            sleep(.5)
        # detach stepper
        self.serial_manager.cmd_stepper_attach(False)
        # check that the number of balanzas matches
        n_balanzas = None
        while n_balanzas is None:
            n_balanzas = self.serial_manager.cmd_hx_n()
            sleep(.5)
        if not self.n_balanzas == n_balanzas:
            raise ValueError(f'System {self.name}: La cantidad de balanzas reportada y la esperada no son iguales')

    def _move_stepper_safe(self, next_pos: Union[Position, int], detach: bool = True) -> bool:
        if isinstance(next_pos, Position):
            next_pos = next_pos.stepper
        elif not isinstance(next_pos, int): # type: ignore
            raise TypeError(f'System {
                            self.name}: next_pos is not of type Position or int. It is of type {type(next_pos)}')
        # save starting position
        start_pos = self.stepper_pos
        # if final position is same as starting, do nothing
        if start_pos == next_pos:
            return True
        # save final position
        self.stepper_pos = next_pos
        self.save_stepper()
        # move stepper
        steps = next_pos - start_pos
        res = self.serial_manager.cmd_stepper(steps, detach=detach)
        # if stepper failed, pretend it never moved. theres is a possibility that it moved anyway. this is dangerous
        if not res:
            self.stepper_pos = start_pos
            self.save_stepper()
            lh.critical(
                'Stepper command failed. Presuming the stepper never moved')
            return False
        return True

    def water(self, position_index: int, intensity: int) -> None:
        if position_index < 0 or position_index >= self.n_balanzas:
            raise IndexError(f'System {self.name}: position index is {position_index} and should be >= 0 and < {self.n_balanzas}')
        position = self.positions[position_index]
        self.serial_manager.cmd_servo(90)
        self._move_stepper_safe(position)
        sleep(.5)
        self.serial_manager.cmd_servo(position.servo)

        tiempo_ms = position.water_time_cruve(intensity)
        pwm = position.water_pwm_curve(intensity)
        self.serial_manager.cmd_pump(tiempo_ms, pwm)

        self.serial_manager.cmd_servo_attach(False)

    def show_all_positions(self, wait_for_user_input: bool = False) -> None:
        for i, position in enumerate(self.positions):
            self.serial_manager.cmd_servo(90)
            self._move_stepper_safe(position)
            self.serial_manager.cmd_servo(position.servo)
            if wait_for_user_input:
                input(f'Posicion {i}. Presiona cualquier tecla para continuar...')
            else:
                sleep(5)
        self.serial_manager.cmd_servo_attach(False)

    def go_home(self, wait_for_user_input: bool) -> None:
        if wait_for_user_input:
            input(f'Stepper current saved position is {self.stepper_pos}. If correct press any button...')
            print('going to home position')
        self.serial_manager.cmd_servo(90)
        self._move_stepper_safe(0)
        self.serial_manager.cmd_servo_attach(False)

    def calibrate_system(self, n: int = 200) -> None:
        self.serial_manager.cmd_stepper_attach(False)
        self.serial_manager.cmd_servo_attach(False)
        balanzas_calibrate(
            balanzas=self.balanzas,
            n_balanzas=self.n_balanzas,
            n=n,
            offset_temp_file=os.path.join(
                self.save_dir, 'config', f'.tmp_balanzas_offset_{self.name}.json')
        )

    def show_system_weights(self, n: Optional[int] = None) -> None:
        print(self.balanzas.read_stats(n))

    def water_test(self, balanza_index: int, intensity: int = 0) -> None:
        if balanza_index < 0 or balanza_index >= self.n_balanzas:
            raise IndexError()
        if self.camera_controller is not None:
            self.camera_controller.start_recording()

        self.water(balanza_index, intensity)

        if self.camera_controller is not None:
            sleep(5)
            self.camera_controller.stop_recording()
            self.camera_controller.tick_until_done()

    def _check_all_right(self, balanza_index: int) -> bool:
        if balanza_index < 0 or balanza_index >= self.n_balanzas:
            raise IndexError(f'System {self.name}: balanza_index is {
                             balanza_index} and should be >= 0 and < {self.n_balanzas}')
        # uso mi libraria para poder indexar mas facilmente
        history_weight = SmartArrayFloat(self.history_weights[balanza_index])
        history_watering = SmartArrayBool(self.history_watering[balanza_index])
        history_intensities = SmartArrayInt(self.history_intensities[balanza_index])
        history_datetimes = SmartArray[datetime](self.history_datetimes, dtype=datetime) # type: ignore
        history_failed_checks = SmartArrayBool(self.history_failed_checks[balanza_index])

        # si el valor es negativo!
        if history_weight[-1] < 0:
            lh.critical((f'check before watering: sistema {self.name}, balanza {balanza_index} '
                        f'-> No paso el chequeo para regar. El peso es negativo ({history_weight[-1]})'))
            return False

        if history_weight[-1] < 50:
            lh.critical((f'check before watering: sistema {self.name}, balanza {balanza_index} '
                        f'-> No paso el chequeo para regar. El peso es menor a 50 gramos({history_weight[-1]}). Se presume que no hay maceta'))

        recent_history = self.history_length - 15
        if len(history_watering) >= recent_history:
            # si viene regando a full hace mucho (no contando fallos del chequeo)
            # no se puede indexar un deque con un slice: deque[1:5] no es posible
            # lo que estoy haciendo es chequear en la historia reciete si en cada sesion de regado ocurrio que:
            #   - se rego
            #   - la intensidad fue mayor o igual a 10
            #   - no hubo un error
            # si para todas las seisiones de regado recientes ocurrio esto, entonces abortar
            if all((w and i >= 10 and not f for w, i, f in
                    zip(history_watering[:recent_history], history_intensities[:recent_history], history_failed_checks[:recent_history]))):
                lh.critical((f'check before watering: sistema {self.name}, balanza {balanza_index} '
                            '-> No paso el chequeo para regar. Esta regando demasiado intenso demasiadas '
                             f'veces (history_watering={history_watering}, history_intensities={
                    history_intensities}, '
                    f'history_weight={history_weight})'))
                return False

        # si vienen muchos valores negativos -> INHABILITAR
        if sum(w < 0 for w in history_weight) > 5:
            lh.critical((f'check before watering: sistema {self.name}, balanza {balanza_index} '
                        f'-> No paso el chequeo para regar. tuvo mas de 5 pesos negativos ({
                history_weight}). '
                'Inhabilitando balanza hasta intervencion manual'))
            self.inhabilitated_balanzas[balanza_index] = True
            return False

        # TODO: seguir completando casos
        return True

    def _update_intensities(self, curr_weights: SmartArrayFloat, watered_last_tick: SmartArrayBool) -> None:
        '''
        the intensities will be updated in the following fashion.
        the difference between current and last weights is calculated.
        if a balanza was watered, the difference should be greater than 'self.watering_min_diff_grams'.
        if it is not grater than that threshold for 'self.watering_max_unchanged_times' consecutive waterings,
        the intensity is bumped by one. if the difference is greater than 'self.watering_max_diff_grams', the
        intensity is decreased by 'self.watering_intensity_lowering_rate'
        '''
        if self.last_weights is None:
            return
        diff = curr_weights - self.last_weights
        low_diff = diff < self.watering_min_diff_grams
        big_diff = diff > self.watering_max_diff_grams
        for i in range(self.n_balanzas):
            if not watered_last_tick[i]:
                continue
            if low_diff[i] and big_diff[i]:
                lh.warning(f'System {self.name}. The weight difference was detected to be low and big at the same time. ({
                           curr_weights}, {self.last_weights}, {watered_last_tick})')
            if low_diff[i]:
                # if it was supposed to be watered but the differece in weight was low
                if self.intensities_unchanged_times[i] >= self.watering_max_unchanged_times:
                    self.intensities[i] += 1
                    self.intensities_unchanged_times[i] = 0
                else:
                    self.intensities_unchanged_times[i] += 1
            elif big_diff[i]:
                # too much water was poured
                self.intensities[i] = max(
                    self.intensities[i] - self.watering_intensity_lowering_rate, 0)
                self.intensities_unchanged_times[i] = 0
        self.last_weights = curr_weights

    def tick(self) -> None:
        # leer datos
        res = None
        while res is None:
            res = self.balanzas.read_stats()
            if res is None:
                sleep(1)
                lh.warning(
                    f'Sistema {self.name}: No se pudo leer las balanzas. Volviendo a intentar...')
        means, stdevs, n_filtered, n_unsuccessful = res
        if not (len(means) == len(stdevs) == self.n_balanzas):
            lh.warning(f'Sistema {self.name}: Al leer se obtuvo una lista de largo {
                len(means)}, {len(stdevs)} cuando hay {self.n_balanzas} balanzas')

        # actualizar watering_schedules, chequear que macetas hay que regar y obtener los objetivos actuales de peso
        for i in range(self.n_balanzas):
            self.watering_schedules[i].update(means[i])
        macetas_to_water = SmartArrayBool(
            ws.should_water(w) for w, ws in zip(means, self.watering_schedules)
        )
        grams_goals = SmartArrayFloat(
            self.watering_schedules[i].current_goal for i in range(self.n_balanzas))

        # agregar datos de mediciones para chequear que todo esta en orden
        all_right = SmartArrayBool.zeros(self.n_balanzas)
        self.history_datetimes.append(datetime.now())
        for i in range(self.n_balanzas):
            self.history_weights[i].append(means[i])
            self.history_watering[i].append(macetas_to_water[i])
            self.history_intensities[i].append(self.intensities[i])
            all_right[i] = self._check_all_right(i)
            self.history_failed_checks[i].append(all_right[i])
        lh.debug(f'Datos de mediciones - sistema {self.name}: pesos=({means}+/-{
                 stdevs}), a_regar={macetas_to_water}, intensidades={self.intensities}')

        # regar
        any_watering = any(macetas_to_water) and any(
            all_right) and not all(self.inhabilitated_balanzas)
        if any_watering:
            if self.camera_controller is not None:
                self.camera_controller.start_recording()

            positions_to_water = [p for i, p in enumerate(
                self.positions) if macetas_to_water[i]]
            best_index_order = stats.get_fastest_path(SmartArrayInt(
                p.stepper for p in positions_to_water), starting_position=self.stepper_pos)

            for i in best_index_order:
                if self.inhabilitated_balanzas[i] or not all_right[i]:
                    continue
                self.water(
                    position_index=i,
                    intensity=self.intensities[i]
                )
                lh.info(f'Tick: Watering {i}, starting with weight {
                        means[i]} +/- {stdevs[i]} and a goal of {grams_goals[i]}')

            if self.camera_controller is not None:
                self.camera_controller.schedule_stop_recording(5)

        # leer dht
        res = self.serial_manager.cmd_dht()
        if res is not None:
            hum, temp = res
        else:
            hum = None
            temp = None

        # guardar lecturas en archivo
        self.file_manager.add_entry(
            means, stdevs, macetas_to_water, n_filtered, n_unsuccessful,
            grams_goals, hum, temp
        )

        # guardar last_weights y actualizar intensidades
        if self.last_weights is None:
            self.last_weights = means
        self._update_intensities(means, macetas_to_water)
