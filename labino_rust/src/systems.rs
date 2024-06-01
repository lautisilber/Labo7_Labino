use std::path::PathBuf;

use crate::balanzas::{Balanzas, BalanzasError};
use serde_derive::{Deserialize, Serialize};
use crate::serial_manager::{DHTResult, SerialManager, SerialManagerError};
use crate::watering_position::{WateringPosition, WateringPositionError};
use crate::watering_schedule::{WateringSchedule, WateringScheduleError};
use crate::utils::{path_is_file, read_file, write_file, now_timestamp};
use lazy_static::lazy_static;
use std::sync::Mutex;

lazy_static! {
    static ref SYSTEM_NAMES: Mutex<Vec<String>> = Mutex::new(Vec::new());
}

fn add_system_name(value: &str) {
    let mut data = SYSTEM_NAMES.lock().unwrap();
    data.push(value.to_string());
}

fn get_system_names() -> Vec<String> {
    let data = SYSTEM_NAMES.lock().unwrap();
    data.clone()
}

#[derive(Debug, thiserror::Error)]
pub enum SystemError {
    #[error("io error")]
    IOError(#[from] std::io::Error),
    #[error("file error")]
    FileError(String, Option<String>),
    #[error("system name error")]
    SystemNameError(String),
    #[error("# balanzas mismatch")]
    NBalanzasMismatchError(String),
    #[error("SerialManagerError")]
    SerialManagerError(#[from] SerialManagerError),
    #[error("serde_json error")]
    SerdeJSONError(#[from] serde_json::Error),
    #[error("balanzas error")]
    BalanzasError(#[from] BalanzasError),
    #[error("index error")]
    IndexError(String),
    #[error("WateringPositionError")]
    WateringPositionError(#[from] WateringPositionError),
    #[error("WateringScheduleError")]
    WateringScheduleError(#[from] WateringScheduleError),
    #[error("SystemTimeError")]
    SystemTimeError(#[from] std::time::SystemTimeError),
}

type Result<T> = std::result::Result<T, SystemError>;

pub struct System {
    name: String,
    n_balanzas: u8,

    positions: Vec<WateringPosition>,
    serial_manager: SerialManager,
    balanzas: Balanzas,
    watering_schedules: Vec<WateringSchedule>,

    stepper_pos: i32,

    intensities: Vec<i32>,

    last_weights: Option<Vec<f32>>,
}

#[derive(Debug, Deserialize, Serialize)]
struct SystemSave {
    name: String,
    n_balanzas: u8,

    positions: Vec<WateringPosition>,
    balanzas: Balanzas,
    watering_schedules: Vec<WateringSchedule>,

    stepper_pos: i32,
}

impl System {
    fn get_dir_from_name(name: &str) -> Result<PathBuf> {
        let pwd = std::env::current_dir()?;
        return Ok(pwd.join("data").join("systems").join(name));
    }

    fn get_system_data_file_from_name(name: &str) -> Result<String> {
        let dir = System::get_dir_from_name(name)?;
        return match dir.join("internal").join("system_data.json").to_str() {
            None => Err(SystemError::FileError("Path is empty".to_owned(), None)),
            Some(path) => Ok(path.to_string())
        }
    }

    fn get_system_log_file_from_name(name: &str) -> Result<String> {
        let dir = System::get_dir_from_name(name)?;
        return match dir.join("log.csv").to_str() {
            None => Err(SystemError::FileError("Path is empty".to_owned(), None)),
            Some(path) => Ok(path.to_string())
        }
    }

    pub fn new(name: &str, n_balanzas: Option<u8>,
               positions: Vec<WateringPosition>, mut serial_manager: SerialManager,
               balanzas: Balanzas, watering_schedules: Vec<WateringSchedule>, stepper_pos: Option<i32>) -> Result<System> {
        {
            let system_names = get_system_names();
            if system_names.contains(&name.to_owned()) {
                return Err(SystemError::SystemNameError(format!("System name {} already registered. Names already registered: {:?}", name, system_names)));
            }
        }
        add_system_name(name);
        let _n_balanzas = match n_balanzas {
            Some(n) => n,
            None => {
                let res = serial_manager.cmd_hx_n()?;
                res as u8
            }
        };
        if positions.len() != _n_balanzas as usize {
            return Err(SystemError::NBalanzasMismatchError(format!("Wrong length of positions vector. n_balanzas: {}, # positions: {}", _n_balanzas, positions.len())));
        }
        if watering_schedules.len() != _n_balanzas as usize {
            return Err(SystemError::NBalanzasMismatchError(format!("Wrong length of watering_schedules vector. n_balanzas: {}, # watering_schedules: {}", _n_balanzas, watering_schedules.len())));
        }
        return Ok(System {
            name: name.to_string(),
            n_balanzas: _n_balanzas,

            positions: positions,
            serial_manager: serial_manager,
            balanzas: balanzas,
            watering_schedules: watering_schedules,

            stepper_pos: match stepper_pos {
                None => {
                    match System::load_system_save(name) {
                        Ok(system_info) => system_info.stepper_pos,
                        Err(_) => 0
                    }
                },
                Some(sp) => sp
            },
            
            intensities: vec![0; _n_balanzas as usize],

            last_weights: None
        });
    }

    fn load_system_save(name: &str) -> Result<SystemSave> {
        let save_file = System::get_system_data_file_from_name(name)?;
        let s = read_file(&save_file)?;
        let system_save: SystemSave = serde_json::from_str(&s)?;
        return Ok(system_save);
    }

    pub fn load(name: &str, serial_manager: SerialManager) -> Result<System> {
        let system_save: SystemSave = System::load_system_save(name)?;
        return System::new(&system_save.name, Some(system_save.n_balanzas),
                           system_save.positions, serial_manager,
                           system_save.balanzas, system_save.watering_schedules, Some(system_save.stepper_pos));
    }

    pub fn save(&self) -> Result<()> {
        let save_file = System::get_system_data_file_from_name(&self.name)?;
        let system_save = SystemSave {
            name: self.name.to_string(),
            n_balanzas: self.n_balanzas,
            positions: self.positions.clone(),
            balanzas: self.balanzas.clone(),
            watering_schedules: self.watering_schedules.clone(),
            stepper_pos: self.stepper_pos
        };
        let s = serde_json::to_string(&system_save)?;
        write_file(&save_file, &s, true)?;
        return Ok(());
    }

    pub fn begin(&mut self) -> Result<()> {
        self.balanzas.begin(&mut self.serial_manager)?;

        while match self.serial_manager.cmd_ok() {
            Ok(v) => v,
            Err(_) => false
        } {
            std::thread::sleep(std::time::Duration::from_millis(500));
        }

        let mut n_balanzas: Option<u8> = None;
        while n_balanzas.is_none() {
            match self.serial_manager.cmd_hx_n() {
                Err(_) => std::thread::sleep(std::time::Duration::from_millis(500)),
                Ok(n) => n_balanzas = Some(n as u8)
            }
        }

        if self.n_balanzas != n_balanzas.unwrap() {
            return Err(SystemError::NBalanzasMismatchError(format!("Wrong n_balanzas. Expected: {}, reported: {}", self.n_balanzas, n_balanzas.unwrap())));
        }

        return Ok(());
    }

    fn stepper_move_safe(&mut self, next_pos: i32, detach: bool) -> Result<()> {
        // save starting position
        let start_pos = self.stepper_pos;

        // if final position is same as starting, do nothing
        if start_pos == next_pos {
            return Ok(());
        }

        // save final position
        self.stepper_pos = next_pos;
        self.save()?;

        // move stepper
        let steps = next_pos - start_pos;
        self.serial_manager.cmd_stepper(steps, detach)?;

        return Ok(());
    }

    pub fn water(&mut self, position_index: usize, intensity: i32) -> Result<()> {
        if position_index >= self.n_balanzas as usize {
            return Err(SystemError::IndexError(format!("position_index index error. max is : {}, reported: {}", self.n_balanzas, position_index)));
        }

        let tiempo_ms: u32;
        let pwm: u8;
        let stepper_pos: i32;

        {
            let pos = &self.positions[position_index];

            stepper_pos = pos.stepper;
            tiempo_ms = pos.water_time_curve.eval(intensity) as u32;
            pwm = pos.water_pwm_curve.eval(intensity) as u8;
        }

        self.serial_manager.cmd_servo(90)?;
        self.stepper_move_safe(stepper_pos, true)?;
        std::thread::sleep(std::time::Duration::from_millis(500));

        self.serial_manager.cmd_pump(tiempo_ms, pwm)?;
        self.serial_manager.servo_attach(false)?;

        return Ok(());
    }

    pub fn tick(&mut self) -> Result<()> {
        // leer datos
        let mut res: Option<(Vec<f32>,Vec<f32>,Vec<usize>,usize)> = None;
        while res.is_none() {
            res = Some(self.balanzas.read_stats(&mut self.serial_manager)?);
            std::thread::sleep(std::time::Duration::from_secs(1));
        }

        let (means, stdevs, n_filtered, n_unsuccessful) = res.unwrap();

        if means.len() != self.n_balanzas as usize || stdevs.len() != self.n_balanzas as usize || n_filtered.len() != self.n_balanzas as usize {
            return Err(SystemError::NBalanzasMismatchError(format!("Length of read didn't match expected {}. means {}, stdevs: {}, n_filtered: {}", self.n_balanzas, means.len(), stdevs.len(), n_filtered.len())));
        }

        // actualizar watering_schedules y chequear que macetas hay que regar
        for (ws, w) in self.watering_schedules.iter_mut().zip(means.iter()) {
            ws.update(*w)?;
        }
        let macetas_to_water: Vec<bool> = self.watering_schedules.iter().zip(&means)
                            .map(|(ws, w)| ws.should_water(*w)).collect();
        let grams_goals: Vec<Option<f32>> = self.watering_schedules.iter().map(|ws| ws.current_goal()).collect();

        // regar
        let any_watering = macetas_to_water.iter().any(|tw| *tw);
        if any_watering {
            let positions_to_water: Vec<&WateringPosition> = self.positions.iter().zip(&macetas_to_water)
                                .filter(|(_, tw)| **tw).map(|(p, _)| p).collect();
            let best_index_order = WateringPosition::find_minimal_distance_path(&positions_to_water, self.stepper_pos)?;

            for i in best_index_order {
                self.water(i, 0)?;
            }
        }

        // leer dht
        let dht = match self.serial_manager.cmd_dht() {
            Err(_) => None,
            Ok(d) => Some(d)
        };


        self.log_tick(&means, &stdevs, &macetas_to_water, &n_filtered, n_unsuccessful, &grams_goals, dht)?;

        return Ok(());
    }

    fn log_tick(&self, means: &[f32], stdevs: &[f32], macetas_to_water: &[bool], n_filtered: &[usize], n_unsuccessful: usize, grams_goals: &[Option<f32>], dht: Option<DHTResult>) -> Result<()> {
        let file_name = System::get_system_log_file_from_name(&self.name)?;
        let file_exists = path_is_file(&file_name);
        let mut content: String = "".to_string();
        if !file_exists {
            content += "time,";
            for i in 0..self.n_balanzas {
                content += &format!("balanza_avg_{},balanza_std_{},balanza_pump_state_{},n_filtered_{},grams_goals_{},", i+1, i+1, i+1, i+1, i+1);
            }
            content += "n_unsuccessful,hum,temp\n";
        }
        let now_str = now_timestamp();
        content += &now_str;
        content += ",";
        for i in 0..(self.n_balanzas as usize) {
            content += &format!("{},{},{},{},", means[i], stdevs[i], if macetas_to_water[i] {1} else {0}, n_filtered[i]);
            match grams_goals[i] {
                None => content += "-,",
                Some(g) => content += &format!("{},", g)
            };
        }
        content += &format!("{},", n_unsuccessful);
        match dht {
            None => content += "-,-\n",
            Some(d) => content += &format!("{},{}\n", d.hum, d.temp),
        }
        write_file(&file_name, &content, false)?;
        return Ok(());
    }
}
