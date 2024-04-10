from serial_manager import SerialManager
from typing import Optional, Tuple
from logging_helper import logger as lh
from statistics import mean, stdev

import json
import os
# try:
#     from tqdm import trange
#     # trange = lambda n: trange(n, leave=False)
# except ImportError:
#     trange = range
from tqdm import trange
from uncertainties import unumpy as unp
from uncertainties import ufloat
from uncertainties import umath
import numpy as np
from type_hints import *


class Balanzas:
    def __init__(self, serial_manager: SerialManager, n_balanzas: int, n_statistics: int=100, n_arduino: int=10, err_threshold: int=5, save_file: str='balanzas.json') -> None:
        self.sm = serial_manager
        self.n_statistics = n_statistics
        self.n_arduino = n_arduino
        self.err_threshold = err_threshold
        self.n_balanzas = n_balanzas
        self.save_file = save_file

        # calibration
        self.offsets: Optional[unp.uarray] = None
        self.slopes: Optional[unp.uarray] = None

        self.load()

    def load(self) -> bool:
        res = False
        if not os.path.isfile(self.save_file):
            lh.info(f'Balanzas: No se pudo cargar calibracion porque no existe el archivo. (Deberia estar en {self.save_file})')
            return res
        with open(self.save_file, 'r') as f:
            try:
                obj = json.load(f)
                offsets = np.array(obj['offsets'], dtype=np_float)
                slopes = np.array(obj['slopes'], dtype=np_float)
                offsets_error = np.array(obj['offsets_error'], dtype=np_float)
                slopes_error = np.array(obj['slopes_error'], dtype=np_float)

                print(offsets, slopes, offsets_error, slopes_error)

                if len(offsets) == len(slopes) == len(offsets_error) == len(slopes_error) == self.n_balanzas:
                    self.offsets = unp.uarray(offsets, offsets_error)
                    self.slopes = unp.uarray(slopes, slopes_error)
                    res = True
                    lh.info('Balanzas: Se cargo la calibracion guardada')
                else:
                    lh.warning('Balanzas: No se pudo cargar calibracion')
                    res = False
            except Exception as err:
                lh.warning(f'Balanzas: No se pudo cargar calibracion con error {err}')
                res = False
            return res

    def save(self) -> bool:
        if any(a is None for a in (self.offsets, self.slopes)):
            lh.warning('Balanzas: No se pudo guardad calibracion porque se calibro aun')
            return False
        obj = {
            'offsets': list(unp.nominal_values(self.offsets)),
            'slopes': list(unp.nominal_values(self.slopes)),
            'offsets_error': list(unp.std_devs(self.offsets)),
            'slopes_error': list(unp.std_devs(self.slopes))
        }
        try:
            with open(self.save_file, 'w') as f:
                json.dump(obj, f)
                # s = json.dumps(obj)
                # f.write(s)
            lh.info('Balanzas: Se guardo la calibracion con exito!')
            return True
        except Exception as err:
            lh.warning(f'Balanzas: No se pudo guardar calibracion ({err})')
            return False

    def read_single_raw(self) -> Optional[np_arr_float_1D]:
        res = self.sm.cmd_hx(self.n_arduino)
        if res is None:
            lh.warning(f'Balanzas: No se pudo leer de las balanzas')
            return None
        if len(res) != self.n_balanzas:
            lh.error(f'Balanzas: No se leyo la cantidad correcta de balanzas ({res})')
            return None
        return np.array(res, dtype=np_float)

    def read_stats_raw(self, n: Optional[int]=None, err_threshold: Optional[float]=None) -> Optional[Tuple[np_arr_float_1D, np_arr_float_1D, np_arr_float_1D, int]]: # [mean, stdev, n_stats_filtered_vals, n_unsuccessful_reads]
        n = self.n_statistics if n is None else n
        err_threshold = self.err_threshold if err_threshold is None else err_threshold
        # si err_threshold <= 0, no se considera, y se toman todos los valores

        vals_l: list[np_arr_float_1D] = list() # [n, self.n_balanzas]

        for _ in trange(n):
            r = self.read_single_raw() # [self.n_balanzas]
            if r is not None:
                vals_l.append(r)
        vals: np_arr_float_2D = np.array(vals_l, dtype=np_float) # [n_error, self.n_balanzas]
        n_completed = len(vals)
        n_error = n-n_completed
        vals_t = vals.T # [self.n_balanzas, n_error]

        # old method of filtering if data is more than err_threshold away from mean
        # means = list(mean(vals_balanza) for vals_balanza in vals_t) #np.mean(vals, axis=0) # should be of size self.n_balanzas
        # cleaned_vals = list() # sums, later to be divided. [self.n_balanzas, ?]
        # for i in range(self.n_balanzas):
        #     cleaned_vals.append(list())
        #     for j in range(n_completed):
        #         val = vals_t[i][j]
        #         diff = abs(val - means[i])
        #         if diff <= err_threshold or err_threshold <= 0:
        #             cleaned_vals[-1].append(val)

        # if any(len(cv) == 0 for cv in cleaned_vals):
        #     lh.error(f'Balanzas: cleaned_vals de forma [self.n_balanzas, ?] tiene una sublista de largo nulo (habiendo empezado con listas de largos {tuple(len(v) for v in vals_t)})')
        #     return None

        ###

        # new method of filtering by quartiles
        # sorted_vals: Tuple[list[float]] = tuple(sorted(v) for v in vals_t)

        # # Calculate quartiles
        # q1 = np.array(tuple(sorted_val[len(sorted_val) // 4] for sorted_val in sorted_vals), dtype=np_float)
        # q3 = np.array(tuple(sorted_val[(3 * len(sorted_val)) // 4] for sorted_val in sorted_vals), dtype=np_float)

        # # Interquartile range (IQR)
        # iqr = q3 - q1

        # # Define the lower and upper bounds
        # lower_bounds = q1 - (1.5 * iqr)
        # upper_bounds = q3 + (1.5 * iqr)

        # # Filter the data based on the bounds
        # filtered_vals: list[list[float]] = [[x for x in sorted_val if lower <= x <= upper] for sorted_val, lower, upper in zip(sorted_vals, lower_bounds, upper_bounds)]

        # cleaned_means = SmartArray([mean(fv) for fv in filtered_vals]) # [self.n_balanzas]
        # cleaned_stdevs = SmartArray([stdev(fv) for fv in filtered_vals]) # [self.n_balanzas]
        # filtered_vals = SmartArray([n_completed-len(fv) for fv in filtered_vals])  # [self.n_balanzas]

        ###

        # filtering by quartile
        q1 = np.percentile(vals_t, 25, axis=1)
        q3 = np.percentile(vals_t, 75, axis=1)

        # Interquartile range (IQR)
        iqr = q3 - q1

        # Define the lower and upper bounds
        lower_bounds = q1 - (1.5 * iqr)
        upper_bounds = q3 + (1.5 * iqr)

        # they are the bounds, but have the same shape as vals_t, so that every value can be compared "in place"
        lower_bounds_extended = np.tile(lower_bounds, vals_t.shape[1]).reshape(vals_t.shape)
        upper_bounds_extended = np.tile(upper_bounds, vals_t.shape[1]).reshape(vals_t.shape)

        # mask of the elements that do not fit in the IQR (True is not valid)
        mask = ~((vals_t > lower_bounds_extended) & (vals_t < upper_bounds_extended))
        for i in range(self.n_balanzas): # to prevent masking everything if all values are the same
            if np.all(mask[i,:]):
                mask[i,:] = np.full(mask.shape[1], False, dtype=np_bool)

        # values, masked by the IQR filter
        vals_masked = np.ma.masked_array(vals_t, mask=mask)

        means_filetered = np.ma.getdata(vals_masked.mean(axis=1), subok=False) # means of non filtered data
        stdevs_filtered = np.ma.getdata(vals_masked.std(axis=1), subok=False) # stdevs of non filtered data
        filtered_vals = mask.sum(axis=1) # number of filtered values per balanza

        lh.debug(f'Balanza: Se leyo las balanzas y hubo {n_error} veces que no se pudo leer del Arduino y {filtered_vals} valores que se descartaron por estadistica')

        return means_filetered, stdevs_filtered, filtered_vals, n_error

    def read_stats(self, n: Optional[int]=None, err_threshold: Optional[float]=None) -> Optional[Tuple[np_arr_float_1D, np_arr_float_1D, np_arr_int_1D, int]]: # [mean, stdev, n_stats_filtered_vals, n_unsuccessful_reads]
        if any(a is None for a in (self.offsets, self.slopes)):
            raise Exception('Balanzas have not been calibrated')
        res = self.read_stats_raw(n, err_threshold)
        if res is None:
            return None
        means_raw, stdevs_raw, filtered_vals, unsuccessful_reads = res

        values_raw = unp.uarray(means_raw, stdevs_raw)
        a = values_raw - self.offsets
        values = (values_raw - self.offsets) / self.slopes

        # means_no_slope = means - self.offsets
        # stdevs_no_slope = sa.sqrt( stdevs**2 + self.offsets_error**2 )

        # means_real = means_no_slope / self.slopes
        # stdevs_real = abs(means_real) * sa.sqrt( (stdevs_no_slope/means_no_slope)**2 + (self.slopes_error/self.slopes)**2 )

        means = unp.nominal_values(values)
        stdevs = unp.std_devs(values)

        lh.debug(f'Balanza: read_stats -> {means}, {stdevs}, {filtered_vals}, {unsuccessful_reads}')
        return means, stdevs, filtered_vals, unsuccessful_reads

    def calibrate_offset(self, n: Optional[int]=None, err_threshold: Optional[float]=None, err_lim: float=1) -> bool:
        n = self.n_statistics*2 if n is None else n
        err_threshold = self.err_threshold if err_threshold is None else err_threshold

        res = self.read_stats_raw(n, err_threshold)
        if res is None:
            lh.error(f'Balanzas: No se pudo calibrar offset porque read_stats_raw() devolvio None')
            return False

        mean, err, _, _ = res
        if any(e > err_lim for e in err):
            lh.error(f'Balanzas: Couldn\'t calibrate offset since there were errors greater than the limit {err_lim}. The errors are {err}')
            return False

        val = unp.uarray(mean, err)
        self.offsets = val
        return True

    def calibrate_slope(self, weights: unp.uarray, n: Optional[int]=None, err_threshold: Optional[float]=None, err_lim: float=10) -> bool:
        n = self.n_statistics*2 if n is None else n
        err_threshold = self.err_threshold if err_threshold is None else err_threshold

        if self.offsets is None:
            lh.warning(f'Balanzas: Can\'t calibrate slope before offset is calibrated')
            return False
        if len(weights) != self.n_balanzas:
            lh.error(f'Balanzas: Couldn\'t calibrate slope since the provided weight list is not of size {self.n_balanzas}. The provided weight list is {weights}')
            return False

        if not isinstance(weights, unp.uarray):
            raise TypeError('weights no es una instancia de unp.uarray')

        res = self.read_stats_raw(n, err_threshold)
        if res is None:
            lh.error(f'Balanzas: No se pudo calibrar slope porque read_stats_raw() devolvio None')
            raise ValueError()

        mean, err, _, _ = res
        val = unp.uarray(mean, err)

        # https://en.wikipedia.org/wiki/Propagation_of_uncertainty
        # slope = dy/dx
        # dy = val - self.offsets
        # dy_err = ua.sqrt(err**2 + self.offsets_error**2)
        # slope = dy / weights
        # slope_err = abs(slope) * ua.sqrt( (dy_err/dy)**2 + (weight_errs/weights)**2 )

        slope = (val - self.offsets) / weights

        if np.any(unp.std_devs(slope) > err_lim):
        # if any(e > err_lim for e in slope.errors()):
            lh.error(f'Balanzas: Couldn\'t calibrate slope since there were errors greater than the limit {err_lim}. The errors are {unp.std_devs(slope)}')
            return False

        self.slopes = slope
        return True

def calibrate(balanzas: Balanzas, n_balanzas: int, n: int=100, offset_temp_file: str='.tmp_balanzas_offset.json'):
    if not os.path.isfile(offset_temp_file):
        input('Remove todo el peso de las balanzas y apreta enter')
        print('Tarando...')
        balanzas.calibrate_offset(n, err_threshold=10000, err_lim=10000)
        try:
            d = {
                'tare_vals': list(unp.nominal_values(balanzas.offsets)),
                'tare_errs': list(unp.std_devs(balanzas.offsets))
            }
            with open(offset_temp_file, 'w') as f:
                json.dump(d, f)
        except Exception as err:
            print(d)
            lh.error(err)
    else:
        with open(offset_temp_file, 'r') as f:
            d = json.load(f)
            offsets = d['tare_vals']
            offsets_errs = d['tare_errs']
            offsets = unp.uarray(offsets, offsets_errs)
            if not len(offsets) == balanzas.n_balanzas:
                raise ValueError()
            balanzas.offsets = offsets
        print('Continuando con una calibracion previa')
    print(balanzas.offsets)
    res_b = False
    while not res_b:
        res = input('Introduci un peso conocido en cada balanza. Esribi los pesos y su error en el siguiente formato: (<numero>,<numero>,...)-<numero error>: ')
        try:
            res = res.replace('(', '').replace(')', '').replace(' ', '')
            weights_str, err_str = res.split('-')
            err_nr = float(err_str)
            weights_errs = [err_nr]*n_balanzas
            weights_str = weights_str.split(',')
            weights = [float(e) for e in weights_str]
            weights = unp.uarray(weights, weights_errs)
            if len(weights) == n_balanzas:
                res_b = True
            else:
                print(f'La cantidad de pesos introducidos fue de {len(weights)}, cuando deberia haber sido {n_balanzas}. Intenta de nuevo')
        except:
            print('No se pudieron convertir los valores introducidos como numeros. Intenta de nuevo')
    print('Calibrando...')
    balanzas.calibrate_slope(weights, n, err_threshold=1000, err_lim=1000)

    if balanzas.save():
        print('Listo!')
    else:
        print('No se pudo guardar el resultado de la calibracion. El resultado es:')
        print('Offsets: ', balanzas.offsets, 'Slopes', balanzas.slopes)

    os.remove(offset_temp_file)

if __name__ == '__main__':
    from time import sleep

    sm = SerialManager()

    n_balanzas = 2

    balanzas = Balanzas(sm, n_balanzas, n_statistics=50, n_arduino=20, err_threshold=30)

    balanzas.load()
