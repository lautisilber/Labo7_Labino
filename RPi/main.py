from balanzas import Balanzas
from balanzas import calibrate as balanzas_calibrate
from serial_manager import SerialManager
from file_manager import FileManager
from logging_helper import logger as lh
from time import sleep
from array import array
import smart_array as sa
import random
# from timer_helper import RepeatedTimer
# from arduino_controller import arduino_reset

def water(sm: SerialManager, index: int, intensity: int):
    #sm.cmd_water(i, tiempo, intensidad)
    sm.cmd_servo_attach(True)
    r = random.randint(-7, 7)
    if index == 0:
        sm.cmd_servo(10 + r)
    elif index == 1:
        sm.cmd_servo(170 + r)
    sm.cmd_servo_attach(False)

    tiempo_ms = min(1400 + (min(max(intensity, 0), 10) * 180), 3200)
    pwm = int(min(67 + (0 if intensity < 10 else (intensity-10) * 3.3 if intensity <= 20 else 100), 100))
    sm.cmd_pump(tiempo_ms, pwm)

    sm.cmd_servo_attach(True)
    sm.cmd_servo(90)
    sm.cmd_servo_attach(False)
    
def tick(sm: SerialManager, balanzas: Balanzas, file_manager: FileManager, n_balanzas: int, grams_goals: array, grams_threashold: float, tiempo: int, intensidad: int) -> bool:
    balanzas_ok = sa.zeros(n_balanzas, 'b')
    all_water_repeat = sa.zeros(n_balanzas, 'L')

    while not all(balanzas_ok):
        res = None
        while res is None:
            res = balanzas.read_stats()
            sleep(5)
            if res is None:
                lh.warning('Main: No se pudo leer las balanzas. Volviendo a intentar...')
        means, stdevs, n_filtered, n_unsuccessful = res

        if not len(means) == len(stdevs):
            raise Exception()

        for i, (w, goal) in enumerate(zip(means, grams_goals)):
            if balanzas_ok[i]: continue

            if w < goal - grams_threashold:
                water(sm, i, all_water_repeat[i])
                lh.info(f'Tick: Watering {i}, starting with weight {w} +/- {stdevs[i]}, goal of {goal} and threashold of {grams_threashold}')
                all_water_repeat[i] += 1
            else:
                balanzas_ok[i] = True

        # res = sm.cmd_dht()
        # if res is not None:
        #     hum, temp = res
        # else:
        #     hum = None
        #     temp = None
        hum = None
        temp = None

        file_manager.add_entry(
            means, stdevs, array('b', [e>0 for e in all_water_repeat]), hum, temp
        )

def calibrate():
    lh.info('Main: Begin')
    sm = SerialManager()
    sm.open()

    n_balanzas = None
    while n_balanzas is None:
        n_balanzas = sm.cmd_hx_n()
        sleep(5)

    balanzas = Balanzas(sm, n_balanzas)

    balanzas_calibrate(sm, balanzas, n_balanzas, 200)


def main():
    lh.info('Main: Begin')
    sm = SerialManager()
    sm.open()


    # arduino_reset()

    while not sm.cmd_ok():
        sleep(1)

    sm.cmd_servo_attach(False)

    # while True:
    #     print(sm.cmd_hx())
    #     sleep(1)

    n_balanzas = None
    while n_balanzas is None:
        n_balanzas = sm.cmd_hx_n()
        sleep(5)

    balanzas = Balanzas(sm, n_balanzas, n_statistics=50, n_arduino=20, err_threshold=30)
    fm = FileManager(n_balanzas)
    
    grams_goals = array('d', (700, 700))
    grams_threshold: float = 10.0

    if not n_balanzas == len(grams_goals):
        raise Exception(f'Main: grams_goals wrong length. correct is {n_balanzas}')

    # t = RepeatedTimer(10, tick, sm, balanzas, fm, n_balanzas, grams_goals, grams_threshold, 1, 50)
    # t.start()

    while True:
        tick(sm, balanzas, fm, n_balanzas, grams_goals, grams_threshold, 1, 50)
        sleep(10)

if __name__ == '__main__':
    main()