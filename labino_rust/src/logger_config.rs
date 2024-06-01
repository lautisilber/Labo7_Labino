use log::LevelFilter;
use log4rs::append::console::{ConsoleAppender, Target};
use log4rs::append::rolling_file::{RollingFileAppender, policy::compound::{
            roll::fixed_window::FixedWindowRoller,
            trigger::size::SizeTrigger,
            CompoundPolicy,
}};
use log4rs::encode::pattern::PatternEncoder;
use log4rs::config::{Appender, Config, Root};
use log4rs::filter::threshold::ThresholdFilter;
use log4rs::Handle;

// example
// https://github.com/estk/log4rs/blob/main/examples/log_to_file.rs
pub fn configure_logger() -> Result<Handle, log::SetLoggerError> {
    /// This is the size at which a new file should be created
    const TRIGGER_FILE_SIZE: u64 = 4 * 1024 * 1024; // 4 megabytes
    /// Number of archive log files to keep
    const LOG_FILE_COUNT: u32 = 10;

    let stdout = ConsoleAppender::builder().target(Target::Stdout).build();

    let pattern = PatternEncoder::new("{d(%Y-%m-%d %H:%M:%S)} {l} {t} {L} - {m}{n}");
    // Create a policy to use with the file logging
    let trigger = SizeTrigger::new(TRIGGER_FILE_SIZE);

    let roller_error = FixedWindowRoller::builder()
        .base(0) // Default Value (line not needed unless you want to change from 0 (only here for demo purposes)
        .build("/logs/archive/errors.{}.log", LOG_FILE_COUNT) // Roll based on pattern and max 3 archive files
        .unwrap();
    let policy_error = CompoundPolicy::new(Box::new(trigger), Box::new(roller_error));
    let rolling_file_error = RollingFileAppender::builder()
        // patterns: https://docs.rs/log4rs/1.3.0/log4rs/encode/pattern/index.html
        .encoder(Box::new(pattern.clone()))
        .append(true)
        .build("./log/errors.{}.log", Box::new(policy_error))
        .unwrap();

    let roller_debug = FixedWindowRoller::builder()
        .base(0) // Default Value (line not needed unless you want to change from 0 (only here for demo purposes)
        .build("/logs/archive/errors.{}.log", LOG_FILE_COUNT) // Roll based on pattern and max 3 archive files
        .unwrap();
    let policy_debug = CompoundPolicy::new(Box::new(trigger), Box::new(roller_debug));
    let rolling_file_debug = RollingFileAppender::builder()
        // patterns: https://docs.rs/log4rs/1.3.0/log4rs/encode/pattern/index.html
        .encoder(Box::new(pattern))
        .append(true)
        .build("./log/debug.{}.log", Box::new(policy_debug))
        .unwrap();

    // Log Trace level output to file where trace is the default level
    // and the programmatically specified level to stderr.
    let config = Config::builder()
        .appender(
            Appender::builder()
                .filter(Box::new(ThresholdFilter::new(LevelFilter::Warn)))
                .build("rolling_file_error", Box::new(rolling_file_error))
        )
        .appender(
            Appender::builder()
                .filter(Box::new(ThresholdFilter::new(LevelFilter::Debug)))
                .build("rolling_file_debug", Box::new(rolling_file_debug))
        )
        .appender(
            Appender::builder()
                .filter(Box::new(ThresholdFilter::new(LevelFilter::Trace)))
                .build("stderr", Box::new(stdout)),
        )
        .build(
            Root::builder()
                .appender("rolling_file_error")
                .appender("rolling_file_debug")
                .appender("stderr")
                .build(LevelFilter::Trace),
        )
        .unwrap();

        let handle = log4rs::init_config(config);

        return handle;
}
