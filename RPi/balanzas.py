from serial_manager import SerialManager
from typing import Optional, Tuple
from logging_helper import logger as lh

import json
import os
try:
    from tqdm import trange
    _have_tqdm = True
except ImportError:
    _have_tqdm = False
from smart_arrays import SmartArrayFloat
from smart_arrays import stats
from smart_arrays.smart_array import SmartArrayInt


def calibration_calc(v: SmartArrayFloat, ve: SmartArrayFloat, o: SmartArrayFloat, oe: SmartArrayFloat, s: SmartArrayFloat, se: SmartArrayFloat) -> tuple[SmartArrayFloat, SmartArrayFloat]:
    '''
    the idea is that with the arrays represented by:
    - value = v +/- ve
    - offset = e +/- oe
    - slope = s +/- se

    we want to do the calculation
        (v-o) / s
    the error associated with this calculation is
        sqrt((ve**2 + oe**2)/(s**2) + (v-o)**2 * abs(se/s**2)**2)
    '''
    res = (v - o) / s
    s2 = s**2
    res_err = (((ve**2 + oe**2)/s2) + ((v - o)**2) * (abs(se/s2)**2))
    return res, res_err


class Balanzas:
    def __init__(self, serial_manager: SerialManager, n_balanzas: int, n_statistics: int=100, n_arduino: int=10, err_threshold: int=5, save_file: str='balanzas.json', use_tqdm: bool=True) -> None:
        self.sm = serial_manager
        self.n_statistics = n_statistics
        self.n_arduino = n_arduino
        self.err_threshold = err_threshold
        self.n_balanzas = n_balanzas
        self.save_file = save_file

        # calibration
        self.offsets: Optional[SmartArrayFloat] = None
        self.offset_errors: Optional[SmartArrayFloat] = None
        self.slopes: Optional[SmartArrayFloat] = None
        self.slope_errors: Optional[SmartArrayFloat] = None

        # visual
        self.use_tqdm = use_tqdm and _have_tqdm

        self.load()

    def load(self) -> bool:
        res = False
        if not os.path.isfile(self.save_file):
            lh.info(f'Balanzas: No se pudo cargar calibracion porque no existe el archivo. (Deberia estar en {self.save_file})')
            return res
        with open(self.save_file, 'r') as f:
            try:
                obj = json.load(f)
                offsets = SmartArrayFloat(obj['offsets'])
                slopes = SmartArrayFloat(obj['slopes'])
                offsets_error = SmartArrayFloat(obj['offsets_error'])
                slopes_error = SmartArrayFloat(obj['slopes_error'])

                # print(offsets, slopes, offsets_error, slopes_error)

                if len(offsets) == len(slopes) == len(offsets_error) == len(slopes_error) == self.n_balanzas:
                    self.offsets = SmartArrayFloat(offsets)
                    self.offset_errors = SmartArrayFloat(offsets_error)
                    self.slopes = SmartArrayFloat(slopes)
                    self.slope_errors = SmartArrayFloat(slopes_error)
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
        if self.slopes is None or self.offsets is None or self.slope_errors is None or self.offset_errors is None:
            lh.warning('Balanzas: No se pudo guardad calibracion porque se calibro aun')
            return False
        obj = {
            'offsets': list(self.offsets),
            'slopes': list(self.slopes),
            'offsets_error': list(self.offset_errors),
            'slopes_error': list(self.slope_errors)
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

    def read_single_raw(self) -> Optional[SmartArrayFloat]:
        res = self.sm.cmd_hx(self.n_arduino)
        if res is None:
            lh.warning(f'Balanzas: No se pudo leer de las balanzas')
            return None
        if len(res) != self.n_balanzas:
            lh.error(f'Balanzas: No se leyo la cantidad correcta de balanzas ({res})')
            return None
        return SmartArrayFloat(res)

    def read_stats_raw(self, n: Optional[int]=None, err_threshold: Optional[float]=None) -> Optional[Tuple[SmartArrayFloat, SmartArrayFloat, SmartArrayInt, int]]: # [mean, stdev, n_stats_filtered_vals, n_unsuccessful_reads]
        n = self.n_statistics if n is None else n
        err_threshold = self.err_threshold if err_threshold is None else err_threshold
        # si err_threshold <= 0, no se considera, y se toman todos los valores

        vals_l: list[SmartArrayFloat] = list() # [n, self.n_balanzas]

        if self.use_tqdm:
            it = trange(n) # type: ignore
        else:
            it = range(n)
        for _ in it:
            r = self.read_single_raw() # [self.n_balanzas]
            if r is not None:
                vals_l.append(r)
        n_completed = len(vals_l)
        n_error = n-n_completed
        vals = [SmartArrayFloat(v) for v in zip(*vals_l)] # [self.n_balanzas, n_error]

        filtered_vals: list[SmartArrayFloat] = [stats.interquartile_range_filter(v) for v in vals] # type: ignore

        means_filtered = SmartArrayFloat(v.mean() for v in filtered_vals)
        stdevs_filtered = SmartArrayFloat(v.stdev() for v in filtered_vals)
        n_filtered_vals = SmartArrayInt(n_completed - len(v) for v in filtered_vals)

        lh.debug(f'Balanza: Se leyo las balanzas y hubo {n_error} veces que no se pudo leer del Arduino y {filtered_vals} valores que se descartaron por estadistica')

        return means_filtered, stdevs_filtered, n_filtered_vals, n_error

    def read_stats(self, n: Optional[int]=None, err_threshold: Optional[float]=None) -> Optional[Tuple[SmartArrayFloat, SmartArrayFloat, SmartArrayInt, int]]: # [mean, stdev, n_stats_filtered_vals, n_unsuccessful_reads]
        if self.slopes is None or self.offsets is None or self.slope_errors is None or self.offset_errors is None:
            raise Exception('Balanzas have not been calibrated')
        res = self.read_stats_raw(n, err_threshold)
        if res is None:
            return None
        means_raw, stdevs_raw, filtered_vals, unsuccessful_reads = res

        # values_raw = UncertaintiesArray(means_raw)
        # values = (values_raw - self.offsets) / self.slopes

        means = (means_raw - self.offsets) / self.slopes
        errors = ((self.offset_errors**2 + stdevs_raw**2) / self.slopes + ((means_raw - self.offsets)**2)*(abs(self.slope_errors/self.slopes**2)**2)).sqrt()

        # means_no_slope = means - self.offsets
        # stdevs_no_slope = sa.sqrt( stdevs**2 + self.offsets_error**2 )

        # means_real = means_no_slope / self.slopes
        # stdevs_real = abs(means_real) * sa.sqrt( (stdevs_no_slope/means_no_slope)**2 + (self.slopes_error/self.slopes)**2 )

        lh.debug(f'Balanza: read_stats -> {means}, {errors}, {filtered_vals}, {unsuccessful_reads}')
        return means, errors, filtered_vals, unsuccessful_reads

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

        # val = UncertaintiesArray(mean, err)
        self.offsets = mean
        self.offset_errors = err
        return True

    def calibrate_slope(self, weights: SmartArrayFloat, weight_errors: SmartArrayFloat, n: Optional[int]=None, err_threshold: Optional[float]=None, err_lim: float=10) -> bool:
        n = self.n_statistics*2 if n is None else n
        err_threshold = self.err_threshold if err_threshold is None else err_threshold

        if self.offsets is None or self.offset_errors is None:
            lh.warning(f'Balanzas: Can\'t calibrate slope before offset is calibrated')
            return False
        if len(weights) != self.n_balanzas:
            lh.error(f'Balanzas: Couldn\'t calibrate slope since the provided weight list is not of size {self.n_balanzas}. The provided weight list is {weights}')
            return False

        if not (isinstance(weights, SmartArrayFloat) and isinstance(weight_errors, SmartArrayFloat)): # type: ignore
            raise TypeError('weights or weight_errors no son una instancia de SmartArrayFloat')

        res = self.read_stats_raw(n, err_threshold)
        if res is None:
            lh.error(f'Balanzas: No se pudo calibrar slope porque read_stats_raw() devolvio None')
            raise ValueError()

        mean, err, _, _ = res
        # val = UncertaintiesArray(mean, err)

        # https://en.wikipedia.org/wiki/Propagation_of_uncertainty
        # slope = dy/dx
        # dy = val - self.offsets
        # dy_err = ua.sqrt(err**2 + self.offsets_error**2)
        # slope = dy / weights
        # slope_err = abs(slope) * ua.sqrt( (dy_err/dy)**2 + (weight_errs/weights)**2 )

        # slope = (val - self.offsets) / weights
        slope, slope_errors = calibration_calc(mean, err, self.offsets, self.offset_errors, weights, weight_errors)

        if any(slope_errors > err_lim):
            lh.error(f'Balanzas: Couldn\'t calibrate slope since there were errors greater than the limit {err_lim}. The errors are {slope_errors}')
            return False

        self.slopes = slope
        self.slope_errors = slope_errors
        return True

def calibrate(balanzas: Balanzas, n_balanzas: int, n: int=100, offset_temp_file: str='.tmp_balanzas_offset.json'):
    if not os.path.isfile(offset_temp_file):
        input('Remove todo el peso de las balanzas y apreta enter')
        print('Tarando...')
        balanzas.calibrate_offset(n, err_threshold=10000, err_lim=10000)
        d = {}
        try:
            if balanzas.offsets is None or balanzas.offset_errors is None:
                raise ValueError('offsets are None')
            d = {
                'tare_vals': list(balanzas.offsets),
                'tare_errs': list(balanzas.offset_errors)
            }
            with open(offset_temp_file, 'w') as f:
                json.dump(d, f)
        except Exception as err:
            lh.error(f'{err}\n{d}')
    else:
        with open(offset_temp_file, 'r') as f:
            d = json.load(f)
            offsets = d['tare_vals']
            offsets_errs = d['tare_errs']
            # offsets = UncertaintiesArray(offsets, offsets_errs)
            if len(offsets) != balanzas.n_balanzas or len(offsets_errs) != balanzas.n_balanzas:
                raise ValueError()
            balanzas.offsets = offsets
            balanzas.offset_errors = offsets_errs
        print('Continuando con una calibracion previa')
    print(balanzas.offsets)
    res_b = False
    weights = None
    weights_errs = None
    while not res_b:
        res = input('Introduci un peso conocido en cada balanza. Esribi los pesos y su error en el siguiente formato: (<numero>,<numero>,...)-<numero error>: ')
        try:
            res = res.replace('(', '').replace(')', '').replace(' ', '')
            weights_str, err_str = res.split('-')
            err_nr = float(err_str)
            weights_errs = SmartArrayFloat.filled(n_balanzas, err_nr) # [err_nr]*n_balanzas
            weights_str = weights_str.split(',')
            weights = SmartArrayFloat(float(e) for e in weights_str)
            # weights = UncertaintiesArray(weights_l, weights_errs)
            if len(weights) == n_balanzas:
                res_b = True
            else:
                print(f'La cantidad de pesos introducidos fue de {len(weights)}, cuando deberia haber sido {n_balanzas}. Intenta de nuevo')
        except:
            print('No se pudieron convertir los valores introducidos como numeros. Intenta de nuevo')
    print('Calibrando...')
    if weights is None or weights_errs is None:
        raise Exception('Weight was None')
    balanzas.calibrate_slope(weights, weights_errs, n, err_threshold=1000, err_lim=1000)

    if balanzas.save():
        print('Listo!')
    else:
        print('No se pudo guardar el resultado de la calibracion. El resultado es:')
        print('Offsets: ', balanzas.offsets, 'Slopes', balanzas.slopes)

    os.remove(offset_temp_file)

if __name__ == '__main__':
    sm = SerialManager()

    n_balanzas = 2

    balanzas = Balanzas(sm, n_balanzas, n_statistics=50, n_arduino=20, err_threshold=30)

    balanzas.load()
