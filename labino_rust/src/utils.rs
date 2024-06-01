use std::{io::Write, path::Path};

pub fn path_exists(path: &str) -> bool {
    return Path::new(path).exists();
}

pub fn path_is_file(path: &str) -> bool {
    return Path::new(path).is_file();
}

pub fn input(msg: Option<&str>) -> std::io::Result<String> {
    match msg {
        None => {},
        Some(s) => print!("{}", s)
    }
    let mut line = std::string::String::new();
    std::io::stdin().read_line(&mut line)?;
    return Ok(line);
}

pub fn write_file(path: &str, content: &str, overwrite: bool) -> std::io::Result<()> {
    let mut file = std::fs::OpenOptions::new().write(true).truncate(overwrite).open(path)?;
    file.write_all(content.as_bytes())?;
    return Ok(());
}

pub fn read_file(path: &str) -> std::io::Result<String> {
    return std::fs::read_to_string(path);
}

pub fn now_timestamp() -> String {
    let system_time = std::time::SystemTime::now();
    let datetime: chrono::DateTime<chrono::Utc> = system_time.into();
    return datetime.format("%Y-%m-%d_%H-%M-%S").to_string();

}