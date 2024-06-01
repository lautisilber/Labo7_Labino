pub mod utils;
pub mod intensity_config;
pub mod watering_position;
pub mod watering_schedule;
pub mod serial_manager;
pub mod balanzas;
pub mod systems;

use std::time::Duration;

use intensity_config::IntensityConfig;
use serial_manager::SerialManager;
use balanzas::Balanzas;
use watering_position::WateringPosition;
use watering_schedule::{WateringSchedule, WateringScheduleStep};

fn main() {
    // /dev/cu.Bluetooth-Incoming-Port
    let sm = SerialManager::new("/dev/ttyACM0", 4800, None, None,
                              None, None, None, None, None, None,
                              None, None).unwrap();
    let balanzas = Balanzas::new("balanza.json", 50, 10).unwrap();

    let intensity_config = IntensityConfig::new(0, 10, 10, 0).unwrap();

    let wp1 = WateringPosition::new(10, 170, intensity_config.clone(), intensity_config.clone());
    let wp2 = WateringPosition::new(30, 20, intensity_config.clone(), intensity_config.clone());

    let watering_position = vec![wp1.clone(), wp1.clone(), wp2.clone()];

    let wss1 = WateringScheduleStep::new(100.0, Duration::from_secs(60), 10.0, Some(10.0)).unwrap();
    let wss2 = WateringScheduleStep::new(200.0, Duration::from_secs(60), 10.0, Some(10.0)).unwrap();
    let watering_schedule_steps = vec![
        wss1.clone(), wss1.clone(), wss2.clone(), wss2.clone(), wss1.clone()
    ];

    let ws1 = WateringSchedule::new(Some(watering_schedule_steps.clone()), true);
    let ws2 = WateringSchedule::new(Some(watering_schedule_steps.clone()), true);
    let watering_schedules = vec![ws1.clone(), ws2.clone(), ws1.clone()];

    let sys = systems::System::new("sys_1", None, watering_position, sm, balanzas, watering_schedules, Some(0)).unwrap();
}
