from balanzas import Balanzas
from balanzas import calibrate as balanzas_calibrate
from serial_manager import SerialManager
from file_manager import FileManager
from logging_helper import logger as lh
from time import sleep
from smart_arrays import SmartArray, UncertaintiesArray
from smart_arrays import smart_array as sa
from typing import Tuple
# from timer_helper import RepeatedTimer
# from arduino_controller import arduino_reset

# change working directory to here
import os
abspath = os.path.abspath(__file__)
dname = os.path.dirname(abspath)
os.chdir(dname)

def tiempo_from_int(intensity: int) -> int:
    return min(1600 + (min(max(intensity, 0), 10) * 180), 2000)

def pwm_from_intensity(intensity: int) -> int:
    return int(min(57 + (0 if intensity < 10 else (intensity-10) * 3.3 if intensity <= 20 else 100), 100))

def water(sm: SerialManager, index: int, intensity: int):
    sm.cmd_servo_attach(True)
    sleep(1)
    if index == 0:
        sm.cmd_servo(25)
    elif index == 1:
        sm.cmd_servo(165)

    tiempo_ms = tiempo_from_int(intensity)
    pwm = pwm_from_intensity(intensity)
    sm.cmd_pump(tiempo_ms, pwm)

    sm.cmd_servo_attach(True)
    sm.cmd_servo(90)
    sm.cmd_servo_attach(False)
    
def tick(sm: SerialManager, balanzas: Balanzas, file_manager: FileManager, n_balanzas: int, grams_goals: SmartArray, grams_threshold: float, intensities: int) -> Tuple[SmartArray, SmartArray]:
    # leer datos
    res = None
    while res is None:
        res = balanzas.read_stats()
        if res is None:
            sleep(5)
            lh.warning('Main: No se pudo leer las balanzas. Volviendo a intentar...')
    vals, n_filtered, n_unsuccessful = res
    if len(vals) != n_balanzas:
        lh.critical(f'Tick: Al leer se obtuvo una lista de largo {len(vals)} cuando hay {n_balanzas} balanzas')
 
    means = vals.values()
    stdevs = vals.errors()

    to_water = means < (grams_goals - grams_threshold)
    for i, (w, intensity) in enumerate(zip(to_water, intensities)):
        if w:
            water(sm, i, intensity)
            lh.info(f'Tick: Watering {i}, starting with weight {means[i]} +/- {stdevs[i]}, goal of {grams_goals[i]} and threashold of {grams_threshold}')

    res = sm.cmd_dht()
    if res is not None:
        hum, temp = res
    else:
        hum = None
        temp = None

    file_manager.add_entry(
        means, stdevs, to_water, n_filtered, n_unsuccessful,
        grams_goals, grams_threshold, hum, temp
    )

    return means, to_water

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


def main() -> None:
    lh.info('Main: Begin')
    sm = SerialManager()
    sm.open()


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
    
    grams_goals = SmartArray((660, 613))
    grams_threshold: float = 2.0

    if not n_balanzas == len(grams_goals):
        raise Exception(f'Main: grams_goals wrong length. correct is {n_balanzas}')

    # t = RepeatedTimer(10, tick, sm, balanzas, fm, n_balanzas, grams_goals, grams_threshold, 1, 50)
    # t.start()

    ### Main Loop ###

    intensities = sa.zeros(n_balanzas, int)
    min_weight_diff = 5
    last_weights, watered_last_tick = tick(sm, balanzas, fm, n_balanzas, grams_goals, grams_threshold, intensities)
    while True:
        weights, watered_last_tick = tick(sm, balanzas, fm, n_balanzas, grams_goals, grams_threshold, intensities)
        weight_diff = abs(weights - last_weights)

        # for i in range(n_balanzas):
        #     if watered_last_tick[i]:
        #         intensities[i] += weight_diff[i] < min_weight_diff[i]
        #     else:
        #         intensities[i] = 0
        intensities = (watered_last_tick * (weight_diff < min_weight_diff)).int()
        sleep(10)

if __name__ == '__main__':
    main()

