pub mod serial_manager;
pub mod balanzas;

use serial_manager::SerialManager;

fn main() {
    // /dev/cu.Bluetooth-Incoming-Port
    let _ = SerialManager::new("/dev/ttyACM0", 4800, None, None,
                              None, None, None, None, None, None,
                              None, None);
}
