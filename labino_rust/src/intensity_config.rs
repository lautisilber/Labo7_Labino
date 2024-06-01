use serde_derive::{Deserialize, Serialize};

#[derive(Debug, thiserror::Error)]
pub enum IntensityConfigError {
    #[error("new error")]
    NewError(String)
}

#[derive(Debug, Deserialize, Serialize, Clone)]
pub struct IntensityConfig {
    initial_value: i32,
    final_value: i32,
    n_steps: u32,
    n_steps_not_incrementing: u32
}

impl IntensityConfig {
    pub fn new(initial_value: i32, final_value: i32, n_steps: u32, n_steps_not_incrementing: u32) -> Result<IntensityConfig, IntensityConfigError> {
        if initial_value > final_value {
            return Err(IntensityConfigError::NewError("Initial value greater than final value".to_owned()));
        }
        if n_steps < n_steps_not_incrementing {
            return Err(IntensityConfigError::NewError("Total steps smaller than steps not incrementing".to_owned()));
        }
        return Ok(IntensityConfig {
            initial_value: initial_value,
            final_value: final_value,
            n_steps: n_steps,
            n_steps_not_incrementing: n_steps_not_incrementing
        });
    }

    pub fn eval(&self, t: i32) -> i32 {
        if t < self.n_steps_not_incrementing as i32 {
            return self.initial_value;
        } else if t >= self.n_steps as i32 {
            return self.final_value;
        }
        let x_range = self.n_steps - self.n_steps_not_incrementing;
        let y_range = self.final_value - self.initial_value;
        let norm_arg = (t - self.n_steps_not_incrementing as i32) as f32 / x_range as f32;
        return (norm_arg * y_range as f32).round() as i32 + self.initial_value;
    }
}