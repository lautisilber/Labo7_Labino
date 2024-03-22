from gpiozero import DigitalInputDevice as InPin
from gpiozero import PWMLED as Led
from typing import Callable
from logging_helper import logger as lh
from enum import Enum, auto

maintenance_cb_t = Callable[[None], None]

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
        self.button_pin = InPin(pin=button_pin, pull_up=True, bounce_time=.5)
        self.led_pin = Led(pin=led_pin, active_high=True)
        self._led_state = Maintenance.LedState.ON
        self.led_on(force=True)
    
    def begin_maintenance(self, callback: maintenance_cb_t) -> None:
        if not isinstance(callback, Callable): raise TypeError()
        def wrapper():
            self.led_blink()
            callback()
        self.button_pin.when_deactivated = wrapper
    
    def end_maintenance(self, callback: maintenance_cb_t) -> None:
        if not isinstance(callback, Callable): raise TypeError()
        def wrapper():
            self.led_blink()
            callback()
        self.button_pin.when_activated = wrapper

    def led_on(self, force: bool=False):
        if force or self._led_state != Maintenance.LedState.ON:
            self.led_pin.on()
            self._led_state = Maintenance.LedState.ON

    def led_pulse(self, force: bool=False):
        if force or self._led_state != Maintenance.LedState.PULSE:
            self.led_pin.pulse(fade_in_time=1, fade_out_time=1, n=None, background=True)
            self._led_state = Maintenance.LedState.PULSE
    
    def led_blink(self, force: bool=False):
        if force or self._led_state != Maintenance.LedState.BLINK:
            self.led_pin.blink(on_time=0.5, off_time=0.5, fade_in_time=0, fade_out_time=0, n=None, background=True)
            self._led_state = Maintenance.LedState.BLINK


    @property
    def button_state(self) -> bool:
        return bool(self.button_pin.value)

if __name__ == '__main__':
    from time import sleep

    m = Maintenance()
    m.begin_maintenance(lambda: print('begin maintenance'))
    m.end_maintenance(lambda: print('end maintenance'))

    while True:
        sleep(1)
        print(m.button_state)
