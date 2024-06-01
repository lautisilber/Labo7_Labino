use crate::intensity_config::IntensityConfig;
use serde_derive::{Deserialize, Serialize};
use itertools::Itertools;

#[derive(Debug, thiserror::Error)]
pub enum WateringPositionError {
    #[error("Index error: {0}")]
    IndexError(String),
}

#[derive(Debug, Deserialize, Serialize, Clone)]
pub struct WateringPosition {
    pub stepper: i32,
    pub servo: u8,
    pub water_time_curve: IntensityConfig,
    pub water_pwm_curve: IntensityConfig
}

impl PartialEq for WateringPosition {
    #[inline]
    fn eq(&self, other: &Self) -> bool {
        return self.stepper == other.stepper && self.servo == other.servo;
    }
}

impl PartialOrd for WateringPosition {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        if self.stepper > other.stepper {
            return Some(std::cmp::Ordering::Greater);
        } else if self.stepper < other.stepper {
            return Some(std::cmp::Ordering::Less);
        } else {
            if self.servo > other.servo {
                return Some(std::cmp::Ordering::Greater);
            } else if self.servo < other.servo {
                return Some(std::cmp::Ordering::Less);
            } else {
                return Some(std::cmp::Ordering::Equal);
            }
        }
    }
}

impl WateringPosition {
    pub fn new(stepper: i32, servo: u8, water_time_curve: IntensityConfig, water_pwm_curve: IntensityConfig) -> WateringPosition {
        return WateringPosition {
            stepper: stepper,
            servo: servo,
            water_time_curve: water_time_curve,
            water_pwm_curve: water_pwm_curve
        };
    }

    fn distance(p1: &WateringPosition, p2: &WateringPosition) -> i32 {
        return (p2.stepper - p1.stepper).abs();
    }

    fn total_distance(l: &[&WateringPosition], path: &[usize]) -> i32 {
        return path.windows(2).fold(0, |acc, window| acc + WateringPosition::distance(&l[window[0]], &l[window[1]]));
    }

    pub fn find_minimal_distance_path(l: &[&WateringPosition], start_pos: i32) -> Result<Vec<usize>, WateringPositionError> {
        let n = l.len();

        let mut start_index: Option<usize> = None;
        for (i, pos) in l.iter().enumerate() {
            if pos.stepper == start_pos {
                start_index = Some(i);
                break;
            }
        }
        if start_index.is_none() {
            return Err(WateringPositionError::IndexError(format!("The starting position ({}) was not found in the list of positoins ({:?})", start_pos, l).to_string()));
        }

        if n == 0 {
            return Err(WateringPositionError::IndexError("l.len() == 0".to_owned()));
        }
        if start_index.unwrap() >= n {
            return Err(WateringPositionError::IndexError(format!("start_index ({}) >= l.len() ({})", start_index.unwrap(), n).to_owned()));
        }

        let mut indices: Vec<usize> = (0..n).collect();
        indices.swap(0, start_index.unwrap()); // Move the start index to the beginning

        let mut min_distance = i32::MAX;
        let mut best_path: Vec<usize> = Vec::new();

        for perm in indices[1..].iter().permutations(n - 1) {
            let mut path = vec![start_index.unwrap()];
            path.extend(perm.iter().copied());
            let dist = WateringPosition::total_distance(l, &path);
            if dist < min_distance {
                min_distance = dist;
                best_path = path;
            }
        }

        return Ok(best_path);
    }
}