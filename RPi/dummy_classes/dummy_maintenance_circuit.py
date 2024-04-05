from typing import Callable
from enum import Enum, auto

maintenance_cb_t = Callable[[], None]

class Maintenance:
    class LedState(Enum):
        ON = auto()
        PULSE = auto()
        BLINK = auto()

    def __init__(self, button_pin: int=24, led_pin: int=23) -> None:
        '''
            button_pin: the pulled-up pin which when grounded, halts the system for maintenance
            led_pin: the pin that drives the status LED
        '''
        self.button_pin = None
        self.led_pin = None
        self._led_state = Maintenance.LedState.ON
        self.led_on(force=True)

    def begin_maintenance(self, callback: maintenance_cb_t) -> None:
        pass

    def end_maintenance(self, callback: maintenance_cb_t) -> None:
        pass

    def led_on(self, force: bool=False):
        self._led_state = Maintenance.LedState.ON

    def led_pulse(self, force: bool=False):
        self._led_state = Maintenance.LedState.PULSE

    def led_blink(self, force: bool=False):
        self._led_state = Maintenance.LedState.BLINK

    @property
    def button_state(self) -> bool:
        return False
