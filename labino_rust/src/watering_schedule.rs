use std::time::{Duration, SystemTime, SystemTimeError};
use serde_derive::{Deserialize, Serialize};


#[derive(Debug, Deserialize, Serialize, Clone)]
pub struct WateringScheduleStep {
    pub weight: f32,
    pub time: Duration,
    pub weight_treshold: f32,
    pub max_weight_difference: Option<f32>    
}

#[derive(Debug, thiserror::Error)]
pub enum WateringScheduleError {
    #[error("Creation error")]
    CreationError(String),
}

impl WateringScheduleStep {
    pub fn new(weight: f32, time: Duration, weight_treshold: f32, max_weight_difference: Option<f32>) -> Result<WateringScheduleStep, WateringScheduleError> {
        if weight_treshold < 0. {
            return Err(WateringScheduleError::CreationError(format!("weight_treshold should be greater than 0. It was {}", weight_treshold)));
        }
        match max_weight_difference {
            None => {},
            Some(m) => {
                if m < 0. {
                    return Err(WateringScheduleError::CreationError(format!("max_weight_difference should be greater than 0 if it is not None. It was {}", m)));
                }
                if weight_treshold - 1. >= m {
                    return Err(WateringScheduleError::CreationError(format!("weight_threshold (- 1 for errors) cannot be greater than max_weight_difference, otherwise goal will never be reached. weight_treshold was {}, and max_weight_difference was {}", weight_treshold, m)));
                }
            }
        }
        return Ok(WateringScheduleStep {
            weight: weight,
            time: time,
            weight_treshold: weight_treshold,
            max_weight_difference: max_weight_difference
        });
    }
}

#[derive(Debug, Deserialize, Serialize, Clone)]
pub struct WateringSchedule {
    steps: Vec<WateringScheduleStep>,
    cyclic: bool,

    curr_step: usize,
    last_step_init_time: SystemTime,
    got_to_weight_goal: bool,
    last_steps_weight: Option<f32>,
    n_cycles: usize
}

impl WateringSchedule {
    pub fn new(steps: Option<Vec<WateringScheduleStep>>, cyclic: bool) -> WateringSchedule {
        return WateringSchedule {
            steps: match steps {
                None => Vec::new(),
                Some(v) => v
            },
            cyclic: cyclic,

            curr_step: 0,
            last_step_init_time: SystemTime::now(),
            got_to_weight_goal: false,
            last_steps_weight: None,
            n_cycles: 0
        }
    }

    #[inline]
    pub fn null_schedule(&self) -> bool {
        return self.steps.is_empty();
    }

    #[inline]
    pub fn n_steps(&self) -> usize {
        return self.steps.len();
    }

    pub fn in_goal(&self, curr_weight: f32) -> bool {
        if self.null_schedule() {
            return true;
        }
        let step = &self.steps[self.curr_step];
        match step.max_weight_difference {
            Some(max_diff) => {
                return step.weight - max_diff <= curr_weight && curr_weight <= step.weight + max_diff;
            },
            None => {
                // como no hay un max_weight_difference, se estara en un goal dependiendo de si el paso anterior tenia un peso objetivo mayor o menor a este
                match self.last_steps_weight {
                    None => return false,  // si no tenemos peso anterior, no podemos hacer nada porque estamos en el primer tick. Esperar hasta que haya un historial
                    Some(last_steps_weight) => {
                        if !self.cyclic && self.n_cycles >= 1 {  // if reached last step and not cyclic
                            // en este caso siempre hay que entender que el objetivo se llega si el peso es mayor al objetivo, dado que queremos mantener la
                            // maceta en este peso. Esto debe ser asi porque no ocurre el unico ccaso en que chequeamos que el peso sea menor, que ocurre cuando
                            // partimos de un peso mayor y debemos esperar hasta que se seque.
                            return curr_weight >= step.weight - step.weight_treshold;
                        } else if step.weight > last_steps_weight {
                            return curr_weight >= step.weight - step.weight_treshold;
                        } else if step.weight < last_steps_weight {
                            return curr_weight <= step.weight - step.weight_treshold;
                        } else {
                            return true;
                        }
                    }
                }
            }
        }
    }

    fn next_step(&mut self, curr_weight: f32) -> () {
        if self.null_schedule() {
            return;
        }
        self.curr_step += 1;
        if self.curr_step >= self.n_steps() {
            if self.cyclic {
                self.curr_step = 0;
                self.n_cycles += 1;
            } else {
                self.curr_step = self.n_steps() - 1;
                self.n_cycles = 1;
            }
        }
        self.got_to_weight_goal = false;
        self.last_steps_weight = Some(curr_weight);
    }

    pub fn curr_step(&self) -> usize {
        return self.curr_step;
    }

    pub fn should_water(&self, curr_weight: f32) -> bool {
        if self.null_schedule() {
            return false;
        }
        if self.in_goal(curr_weight) {
            return false;
        }
        let step = &self.steps[self.curr_step];
        return step.weight > curr_weight;
    }

    pub fn update(&mut self, curr_weight: f32) -> Result<(), SystemTimeError> {
        if self.null_schedule() {
            return Ok(());
        }
        let step = &self.steps[self.curr_step];
        if self.got_to_weight_goal {
            self.got_to_weight_goal = true;
            self.last_step_init_time = SystemTime::now();
            self.update(curr_weight)?;
        } else {
            if !step.time.is_zero() {
                self.next_step(curr_weight);
            } else {
                if self.last_step_init_time.elapsed()? > step.time {
                    self.next_step(curr_weight);
                }
            }
        }
        match self.last_steps_weight {
            None => {
                self.last_steps_weight = Some(curr_weight);
            },
            Some(_) => {}
        }
        return Ok(());
    }

    pub fn current_goal(&self) -> Option<f32> {
        if self.null_schedule() {
            return None;
        } else {
            return Some(self.steps[self.curr_step].weight);
        }
    }
}