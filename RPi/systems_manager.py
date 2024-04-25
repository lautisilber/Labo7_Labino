from systems import System
from config import TEST
if not TEST:
    from maintenance_circuit import Maintenance
else:
    from dummy_classes.dummy_maintenance_circuit import Maintenance
from typing import Optional, Sequence, Union
from time import sleep

class SystemsManager:
    def __init__(self, systems: Union[Sequence[System], System]) -> None:
        if isinstance(systems, System):
            self.systems = (systems,)
        elif isinstance(systems, Sequence): # type: ignore
            self.systems = tuple(systems)
        else:
            raise TypeError(f'systems was not a sequence of System or System. Instead was {type(systems)}')
        if not all(isinstance(s, System) for s in self.systems): # type: ignore
            raise TypeError(f'systems was not a sequence of System or System. Instead was {type(systems)}')
        self.n_systems = len(self.systems)

        # flags
        self.halt_flag = False
        self.maintenance = Maintenance()
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
        if index < 0 or index >= self.n_systems:
            raise IndexError(f'{index} < 0 or {index} >= {self.n_systems}')
        self.systems[index].begin()

    def begin(self) -> None:
        for s in self.systems:
            s.begin()

    def show_all_positions(self, index: int, wait_for_user_input: bool=False) -> None:
        if index < 0 or index > self.n_systems:
            raise IndexError(f'{index} < 0 or {index} >= {self.n_systems}')
        self.systems[index].show_all_positions()

    def go_home(self, index: int, wait_for_user_input: bool) -> None:
        if index < 0 or index > self.n_systems:
            raise IndexError(f'{index} < 0 or {index} >= {self.n_systems}')
        self.systems[index].go_home(wait_for_user_input)

    def calibrate_system(self, index: int, n: int=200) -> None:
        if index < 0 or index > self.n_systems:
            raise IndexError(f'{index} < 0 or {index} >= {self.n_systems}')
        self.systems[index].calibrate_system()

    def show_system_weights(self, index: int, n: Optional[int]=None) -> None:
        if index < 0 or index > self.n_systems:
            raise IndexError(f'{index} < 0 or {index} >= {self.n_systems}')
        self.systems[index].show_system_weights(n)

    def water_test(self, system_index: int, balanza_index: int, intensity: int=0) -> None:
        if system_index < 0 or system_index > self.n_systems:
            raise IndexError(f'{system_index} < 0 or {system_index} >= {self.n_systems}')
        s = self.systems[system_index]
        if balanza_index < 0 or balanza_index >= s.n_balanzas:
            raise IndexError(f'{balanza_index} < 0 or {balanza_index} >= {s.n_balanzas}')
        s.water_test(balanza_index, intensity)

    def loop(self):
        while True:
            for s in self.systems:
                if not self.check_halt():
                    s.tick()
                else:
                    sleep(.1)
