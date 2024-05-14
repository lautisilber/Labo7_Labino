use serde_derive::{Deserialize, Serialize};
use std::io::{Read, Write};
use std::path::Path;
use std::fs::File;

use crate::serial_manager::SerialManager;

type GenError = Box<dyn std::error::Error>;
type Result<T> = std::result::Result<T, GenError>;

#[derive(Debug, Deserialize, Serialize)]
pub struct Balanzas {
    n_balanzas: Option<u8>,
    n_statistics: usize,
    n_arduino: u8,
    save_file: String,

    offsets: Option<Vec<f32>>,
    offset_errors: Option<Vec<f32>>,
    slopes: Option<Vec<f32>>,
    slope_errors: Option<Vec<f32>>
}

fn transpose<T: Clone + Copy>(matrix: Vec<Vec<T>>) -> Vec<Vec<T>> {
    if matrix.is_empty() {
        return Vec::new();
    }

    let num_rows = matrix.len();
    let num_cols = matrix[0].len();

    let mut result = vec![vec![matrix[0][0].clone(); num_rows]; num_cols];

    for i in 0..num_rows {
        for j in 0..num_cols {
            result[j][i] = matrix[i][j];
        }
    }

    return result;
}

fn mean_and_stdev(v: &Vec<f32>) -> (f32, f32) {
    let sum: f32 = v.iter().sum();
    let count = v.len() as f32;
    let mean = sum / count;
    let variance: f32 = v.iter().map(|x| (x - mean).powi(2)).sum::<f32>() / (count - 1.);
    let stdev = variance.sqrt();
    return (mean, stdev);
}

impl Balanzas {
    pub fn save(&self) -> Result<()> {
        let s = serde_json::to_string(self)?;
        let mut file = std::fs::OpenOptions::new().write(true).truncate(true).open(&self.save_file)?;
        file.write_all(s.as_bytes())?;
        return Ok(());
    }

    pub fn new(save_file: &str, n_statistics: usize, n_arduino: u8) -> Result<Balanzas>
    {
        if Path::new(save_file).is_file() {
            // load
            let mut file = File::open(save_file)?;
            let mut contents = String::new();
            file.read_to_string(&mut contents)?;
    
            let balanzas: Balanzas = serde_json::from_str(&contents).expect(&format!("Couldn't balanzas. File content was \"{}\"", contents));
            return Ok(balanzas);
        } else {
            // create new
            return Ok(Balanzas{
                n_balanzas: None,
                n_statistics: n_statistics,
                n_arduino: n_arduino,
                save_file: save_file.to_owned(),
                offsets: None,
                offset_errors: None,
                slopes: None,
                slope_errors: None
            });
        }
    }

    fn read_single_raw(&self, serial_manager: &mut SerialManager) -> Result<Vec<f32>> {
        match self.n_balanzas {
            None => return Err(Box::from("Can't execute read_single_raw since n_balanzas was not set")),
            Some(n_balanzas) => {
                let raw_values = serial_manager.cmd_hx(self.n_arduino)?;
                if raw_values.len() != n_balanzas as usize {
                    return Err(Box::from(format!("Error reading single raw balanza. The length of the weights array didn't match n_balanzas (raw_values: {:?}, n_balanzas: {}", raw_values, n_balanzas)));
                }
                return Ok(raw_values);
            }
        }
    }

    fn read_stats_raw(&self, serial_manager: &mut SerialManager) -> Result<(Vec<f32>,Vec<f32>,Vec<usize>,usize)> {
        // return [mean, stdev, n_stats_filtered_vals, n_unsuccessful_reads]
        let mut list_of_raw_vals: Vec<Vec<f32>> = Vec::new();
        for _ in 0..self.n_statistics {
            match self.read_single_raw(serial_manager) {
                Err(_) => {},
                Ok(raw) => {
                    list_of_raw_vals.push(raw)
                }
            }
        }
        if list_of_raw_vals.is_empty() {
            return Err(Box::from("Error executing read_stats_raw because no reading could be made"));
        }
        let n_completed = list_of_raw_vals.len();
        let n_error = self.n_statistics as usize - n_completed;
        let vals = transpose(list_of_raw_vals);

        let mut means_filtered: Vec<f32> = Vec::with_capacity(self.n_balanzas.unwrap() as usize);
        let mut stdevs_filtered: Vec<f32> = Vec::with_capacity(self.n_balanzas.unwrap() as usize);
        let mut n_filtered_vals: Vec<usize> = Vec::with_capacity(self.n_balanzas.unwrap() as usize);

        for n in 0..self.n_balanzas.unwrap() {
            let vals_ref = &vals[n as usize];
            let mut vals_sorted = (*vals_ref).clone();
            vals_sorted.sort_by(|a,b| a.partial_cmp(b).unwrap_or(std::cmp::Ordering::Less));

            let q1 = vals_sorted[vals_sorted.len()/4];
            let q3 = vals_sorted[3*vals_sorted.len()/4];

            let iqr = q3 - q1;

            let lower = q1 - (1.5 * iqr);
            let upper = q3 + (1.5 * iqr);

            // this has all values that were not filtered from the balanza n
            // note that after into_iter, vals_sorted cannot be used
            // since we're calculating statistics, order does not matter and we can use vals_sorted instead of vals_ref
            let filtered_vals: Vec<f32> = vals_sorted.into_iter().filter(|v| lower < *v && *v < upper).collect();

            let mean_stdev = mean_and_stdev(&filtered_vals);

            means_filtered[n as usize] = mean_stdev.0;
            stdevs_filtered[n as usize] = mean_stdev.1;
            n_filtered_vals[n as usize] = vals_ref.len() - filtered_vals.len();
        }

        return Ok((means_filtered, stdevs_filtered, n_filtered_vals, n_error));
    }

    pub fn read_stats(&self, serial_manager: &mut SerialManager) -> Result<(Vec<f32>,Vec<f32>,Vec<usize>,usize)> {
        // return [mean, stdev, n_stats_filtered_vals, n_unsuccessful_reads]
        if self.offsets.is_none() || self.offset_errors.is_none() || self.slopes.is_none() || self.slopes.is_none() {
            return Err(Box::from("Error in read_stats. offset, offset_errors, slope, or slope_errors is None"));
        }

        let res = self.read_stats_raw(serial_manager)?;
        let mut means: Vec<f32> = Vec::with_capacity(self.n_balanzas.unwrap() as usize);
        let mut stdevs: Vec<f32> = Vec::with_capacity(self.n_balanzas.unwrap() as usize);
        for n in 0..self.n_balanzas.unwrap() {
            let n_u = n as usize;
            let mean = res.0[n_u];
            let stdev = res.1[n_u];
            let offset = self.offsets.as_ref().unwrap()[n_u];
            let offset_error = self.offset_errors.as_ref().unwrap()[n_u];
            let slope = self.slopes.as_ref().unwrap()[n_u];
            let slope_error = self.slope_errors.as_ref().unwrap()[n_u];

            means[n_u] = slope * mean * offset;
            stdevs[n_u] = ( (offset_error.powi(2) + stdev.powi(2)) / slope +  (mean - offset).powi(2) * (slope_error / slope.powi(2)).powi(2) ).sqrt();
        }
        return Ok((means, stdevs, res.2, res.3));
    }

    pub fn calibrate_offset(&mut self, serial_manager: &mut SerialManager) -> Result<()> {
        let res = self.read_stats_raw(serial_manager)?;
        self.offsets = Some(res.0);
        self.offset_errors = Some(res.1);
        return Ok(());
    }

    pub fn calibrate_slope(&mut self, serial_manager: &mut SerialManager, weights: &Vec<f32>, weight_errors: &Vec<f32>) -> Result<()> {
        if weights.len() != self.n_balanzas.unwrap() as usize || weight_errors.len() != self.n_balanzas.unwrap() as usize {
            return Err(Box::from(format!("Error in calibrate_slope. weights or weight_errors has wrong size. Weights size is {} and weight_errors size is {} but should be {}", weights.len(), weight_errors.len(), self.n_balanzas.unwrap())));
        }
        match &self.offsets {
            None => return Err(Box::from("Can't calibrate slope since offsets is None")),
            Some(offsets) => {
                match &self.offset_errors {
                    None => return Err(Box::from("Can't calibrate slope since offset_errors is None")),
                    Some(offset_errors) => {
                        let res = self.read_stats_raw(serial_manager)?;
                        // value = slope * mean + offset
                        // error = sqrt(offset_error**2 + slope_error**2 * mean**2 + slope**2 * stdev**2)

                        let mut slopes: Vec<f32> = Vec::with_capacity(self.n_balanzas.unwrap() as usize);
                        let mut slope_errors: Vec<f32> = Vec::with_capacity(self.n_balanzas.unwrap() as usize);
                        let means_raw = res.0;
                        let stdevs_raw = res.1;

                        // slope = (value - offset) / mean
                        // error = sqrt((offset_error**2 + stdevs_raw**2) / slope_error**2 + (mean_raw - offset)**2 * (stdevs_raw / (mean_raw**2))**2)

                        for n in 0..self.n_balanzas.unwrap() {
                            let n_u = n as usize;
                            slopes[n_u] = (weights[n_u] - offsets[n_u]) / means_raw[n_u];
                            slope_errors[n_u] = ( (offset_errors[n_u] + weight_errors[n_u]) / means_raw[n_u].powi(2) + (means_raw[n_u] - offset_errors[n_u]).powi(2) * (stdevs_raw[n_u] / means_raw[n_u].powi(2)).powi(2) ).sqrt();
                        }

                        self.slopes = Some(slopes);
                        self.slope_errors = Some(slope_errors);

                        return Ok(());
                    }
                }
            }
        }
    }
}

