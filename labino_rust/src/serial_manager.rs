use serialport::{available_ports, ClearBuffer, DataBits, FlowControl, Parity, SerialPort, StopBits};
use std::{thread::sleep, time::{Duration, SystemTime}};
use serde_derive::Deserialize;

// Change the alias to use `Box<dyn error::Error>`.
type GenError = Box<dyn std::error::Error>;
type Result<T> = std::result::Result<T, GenError>;

pub struct SerialManager {
    port: Box<dyn SerialPort>,
    n_retries: u8,
    delay: Duration,
    timeout_long: Duration,
    end_char: u8,
    rcv_str: String,

    last_hx_n: Option<usize>,
    last_hx_n_time: Option<SystemTime>
}

fn char_to_ascii(ch: char) -> Option<u8> {
    if ch.is_ascii() {
        Some(ch as u8)
    } else {
        None
    }
}

fn ascii_to_char(code: u8) -> Option<char> {
    // if code <= 255 {
    //     Some(code as char)
    // } else {
    //     None
    // }
    return Some(code as char);
}

#[derive(Debug, Deserialize)]
pub struct DHTResult {
    hum: f32,
    temp: f32
}

impl SerialManager {
    pub fn new(port: &str, baud_rate: u32, data_bits: Option<DataBits>, flow_control: Option<FlowControl>, parity: Option<Parity>,
        stop_bits: Option<StopBits>, timeout: Option<Duration>, n_retries: Option<u8>, delay: Option<Duration>, timeout_long: Option<Duration>,
        end_char: Option<u8>, rcv_str: Option<&str>) -> Result<SerialManager> {
        
        let existing_ports = available_ports()?;
        if existing_ports.is_empty() {
            return Err(Box::from("No ports available"));
        }

        if existing_ports.iter().any(|p| p.port_name == port) {
            return Err(Box::from("Port not found"));
        }
        
        let serial_port = serialport::new(port, baud_rate)
            .timeout(timeout.unwrap_or(Duration::from_millis(3000)))
            .data_bits(data_bits.unwrap_or(DataBits::Eight))
            .flow_control(flow_control.unwrap_or(FlowControl::None))
            .parity(parity.unwrap_or(Parity::Even))
            .stop_bits(stop_bits.unwrap_or(StopBits::One))
            .open()
            .expect("Failed to open port");

        if end_char.is_some() {
            // if end_char.unwrap() > 255 {
            //     return Err(Box::from("end_char is not an ascii character"));
            // }
            return Err(Box::from("end_char is not an ascii character"));
        }

        return Ok(SerialManager{
            port: serial_port,
            n_retries: n_retries.unwrap_or(3),
            delay: delay.unwrap_or(Duration::from_millis(500)),
            timeout_long: timeout_long.unwrap_or(Duration::from_secs(30)),
            end_char: end_char.unwrap_or(char_to_ascii('\n').unwrap()),
            rcv_str: match rcv_str {
                Some(s) => s.to_owned(),
                None => "rcv".to_owned()
            },
            last_hx_n: None,
            last_hx_n_time: None
        });
    }

    fn read(&mut self) -> Result<String> {
        let mut buf: String = std::string::String::new();
        self.port.read_to_string(&mut buf)?;
        return Ok(buf);
    }

    fn write(&mut self, msg: &str) -> Result<usize> {
        return Ok(self.port.write(msg.as_bytes())?);
    }

    fn flush(&mut self) -> Result<()> {
        // waits until outgoing (write) buffer is sent
        let res: Result<()> = self.port.flush().map_err(|e| e.into());
        return res;
    }

    fn clear_buffer(&self, buf: Option<ClearBuffer>) -> Result<()> {
        return self.port.clear(buf.unwrap_or(ClearBuffer::All)).map_err(|e| e.into());
    }

    fn in_waiting(&self) -> Result<u32> {
        return self.port.bytes_to_read().map_err(|e| e.into());
    }

    pub fn send_command_wait_response(&mut self, cmd: &str) -> Result<String> {
        // reset all buffers
        self.flush()?;
        self.clear_buffer(None)?;

        // check that cmd is correctly ended
        let mut owned_cmd = cmd.to_owned();
        if !owned_cmd.ends_with(ascii_to_char(self.end_char).unwrap()) {
            owned_cmd.push_str(&ascii_to_char(self.end_char).unwrap().to_string());
        }
    
        self.write(&owned_cmd)?;
        if !self.delay.is_zero() {
            sleep(self.delay);
        }

        // read and wait for second read if requested
        let mut res = self.read()?;
        if res == self.rcv_str {
            let now = SystemTime::now();

            while now.elapsed()? < self.timeout_long && self.in_waiting()? == 0 {
                sleep(self.delay);
            }
            res = self.read()?;
        }

        // check that the result wasn't empty or was an error message
        res = res.trim().to_string();
        if res.is_empty() {
            return Err(Box::from("Arduino responded with empty string"));
        }
        if res.contains("ERROR") {
            return Err(Box::from(format!("Arduino responded with the error: \"{}\"", res)));
        }

        return Ok(res);
    }

    pub fn send_command_wait_response_retries(&mut self, cmd: &str) -> Result<String> {
        let mut last_error: GenError = Box::from("No error");
        for _ in 0..self.n_retries {
            let res = self.send_command_wait_response(cmd);
            match res {
                Ok(r) => {
                    if !r.is_empty() {
                        return Ok(r);
                    }
                },
                Err(e) => last_error = e
            }
        }
        return Err(last_error);
    }

    pub fn cmd_ok(&mut self) -> Result<bool> {
        return Ok(self.send_command_wait_response_retries("ok")? == "OK");
    }

    pub fn cmd_hx(&mut self, n: u8) -> Result<Vec<f32>> {
        let res = self.send_command_wait_response_retries(format!("hx {}", n).as_str())?;
        if !res.starts_with('[') || !res.ends_with(']') {
            return Err(Box::from(format!("Arduino responded to cmd_hx with a bad format: \"{}\"", res)));
        }

        let mut weights: Vec<f32> = Vec::new();
        let list_elems = res[1..res.len()-1].split(',');
        for elem in list_elems {
            match elem.trim().parse::<f32>() {
                Ok(weight) => weights.push(weight),
                Err(e) => return Err(Box::from(format!("Arduino responded to cmd_hx with a bad format: \"{}\". The error was \"{}\"", res, e)))
            }
        }

        return Ok(weights);
    }

    pub fn cmd_hx_single(&mut self, index: u8, n: u8) -> Result<f32> {
        let res = self.send_command_wait_response_retries(format!("hx_single {} {}", n, index).as_str())?;
        
        return match res.trim().parse::<f32>() {
            Ok(weight) => Ok(weight),
            Err(e) => Err(Box::from(format!("Arduino responded to cmd_hx_single with a bad format: \"{}\". The error was \"{}\"", res, e)))
        }
    }

    pub fn cmd_hx_n(&mut self) -> Result<usize> {
        if self.last_hx_n_time.is_none() {
            self.last_hx_n_time = Some(SystemTime::now());
        }
        if self.last_hx_n_time.unwrap().elapsed()? > Duration::from_secs(5 * 60) || self.last_hx_n.is_none() {
            self.last_hx_n = Some(self.send_command_wait_response_retries("ok")?.parse::<usize>()?);
        }

        return Ok(self.last_hx_n.unwrap());
    }

    pub fn cmd_dht(&mut self) -> Result<DHTResult> {
        let res = self.send_command_wait_response_retries("dht")?;
        let dht_res: DHTResult = serde_json::from_str(&res).expect(&format!("Couldn't parse dht result. Arduino responded with \"{}\"", res));
        return Ok(dht_res);
    }

    pub fn cmd_stepper(&mut self, steps: i32) -> Result<()> {
        let res = self.send_command_wait_response_retries(&format!("stepper {}", steps))?;
        if res != "OK" {
            return Err(Box::from(format!("Error on stepper command. Arduino responded with \"{}\"", res)));
        } else {
            return Ok(());
        }
    }

    pub fn cmd_servo(&mut self, angle: u8) -> Result<()> {
        if angle < 1 || angle > 179 {
            return Err(Box::from(format!("Bad argument for cmd_servo. Angle should be in range [1, 179]. It was \"{}\"", angle)));
        }
        let res = self.send_command_wait_response_retries(&format!("servo {}", angle))?;
        if res.parse::<u8>()? != angle {
            return Err(Box::from(format!("Error on servo command. Arduino responded with \"{}\". (angle was {})", res, angle)));
        } else {
            return Ok(());
        }
    }

    pub fn cmd_pump(&mut self, time_ms: u32, intensity: u8) -> Result<()> {
        if intensity > 100 {
            return Err(Box::from(format!("Bad argument for cmd_pump. Intensity showld be equal or lower than 100. It was \"{}\"", intensity)));
        }
        let res = self.send_command_wait_response_retries(&format!("pump {} {}", time_ms, intensity))?;
        if res != "OK" {
            return Err(Box::from(format!("Error on pump command. Arduino responded with \"{}\". (time_ms was {} and intensity was {})", res, time_ms, intensity)));
        } else {
            return Ok(());
        }
    }

    pub fn stepper_attach(&mut self, attach: bool) -> Result<()> {
        let _ = self.send_command_wait_response_retries(&format!("stepper_attach {}", (attach as u8).to_string()))?;
        return Ok(());
    }

    pub fn servo_attach(&mut self, attach: bool) -> Result<()> {
        let _ = self.send_command_wait_response_retries(&format!("servo_attach {}", (attach as u8).to_string()))?;
        return Ok(());
    }
}