from gpiozero import DigitalOutputDevice as Pin
from time import sleep

reset_arduino_pin = Pin(18, active_high=False, initial_value=False)

def arduino_reset():
    reset_arduino_pin.off()
    sleep(1)
    reset_arduino_pin.on()
    sleep(5)

if __name__ == '__main__':
    arduino_reset()