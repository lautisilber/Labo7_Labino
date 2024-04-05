from systems import System, Position, IntensityConfig, SerialManagerInfo, BalanzasInfo, WateringScheme, WateringSchemeStep, CameraControllerInfo
from systems_manager import SystemsManager
from typing import Optional

# change working directory to here
import os
abspath = os.path.abspath(__file__)
dname = os.path.dirname(abspath)
os.chdir(dname)
del abspath, dname


def run(systems_manager: SystemsManager) -> None:
    systems_manager.begin()
    systems_manager.loop()


def show_positions(systems_manager: SystemsManager, system_index: int, wait_for_user_input: bool) -> None:
    systems_manager.begin_single(system_index)
    systems_manager.show_all_positions(system_index, wait_for_user_input)


def go_home(systems_manager: SystemsManager, system_index: int, wait_for_user_input: bool) -> None:
    systems_manager.begin_single(system_index)
    systems_manager.go_home(system_index, wait_for_user_input)


def calibrate(systems_manager: SystemsManager, system_index: int, n: int = 200) -> None:
    systems_manager.begin_single(system_index)
    systems_manager.calibrate_system(system_index, n)


def show_weights(systems_manager: SystemsManager, system_index: int, n: Optional[int] = None) -> None:
    systems_manager.begin_single(system_index)
    systems_manager.show_system_weights(system_index, n)


def water_test(systems_manager: SystemsManager, system_index: int, balanza_index: int, intensity: int = 0) -> None:
    systems_manager.begin_single(system_index)
    systems_manager.water_test(system_index, balanza_index, intensity)


def main() -> None:
    sys_1 = System(
        positions=(
            Position(0, 179, IntensityConfig(1600, 3000, 15, 5), IntensityConfig(53, 75, 30, 15)),    # pos 1
            Position(0, 1, IntensityConfig(1600, 3400, 15, 5), IntensityConfig(55, 80, 30, 15)),      # pos 2
            Position(4800, 1, IntensityConfig(1700, 3400, 15, 5), IntensityConfig(53, 80, 30, 15)),   # pos 3
            Position(5000, 172, IntensityConfig(1700, 3400, 15, 5), IntensityConfig(53, 80, 30, 15)),  # pos 4
            Position(9700, 162, IntensityConfig(1900, 3500, 15, 2), IntensityConfig(53, 80, 30, 15)),  # pos 5
            Position(9700, 20, IntensityConfig(1800, 3400, 15, 2), IntensityConfig(53, 80, 30, 15))   # pos 6
        ),
        sm_info=SerialManagerInfo(
            port='/dev/ttyACM0',
            baud_rate=9600,
            parity='E'
        ),
        balanzas_info=BalanzasInfo(
            n_statistics=50,
            n_arduino=10
        ),
        watering_schemes=tuple([WateringScheme((WateringSchemeStep(670, 0), WateringSchemeStep(610, 0), WateringSchemeStep(670, 0)), cyclic=False)]*6),
        cc_info=CameraControllerInfo(),
        n_balanzas=6,
        name='system_1',
        save_dir=os.path.join('systems', 'system_1'),
    )

    systems_manager = SystemsManager((sys_1,))

    # calibrate(systems_manager, 0, 500)

    # show_positions(systems_manager, 0, True)

    # go_home(systems_manager, 0, True)

    # show_weights(systems_manager, 0, 10)

    # water_test(systems_manager, system_index=0, balanza_index=1, intensity=12)

    run(systems_manager)


if __name__ == '__main__':
    main()
