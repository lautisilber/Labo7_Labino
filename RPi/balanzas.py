from serial_manager import SerialManager
from typing import List, Optional, Tuple, Union
from logging_helper import logger as lh
from smart_arrays import UncertaintiesArray, SmartArray
from smart_arrays import uncertainties_array as ua
from smart_arrays import smart_array as sa
from statistics import mean, stdev
import json
import os
try:
    from tqdm import trange
    # trange = lambda n: trange(n, leave=False)
except ImportError:
    trange = range


class Balanzas:
    def __init__(self, serial_manager: SerialManager, n_balanzas: int, n_statistics: int=100, n_arduino: int=10, err_threshold: int=5, save_file: str='balanzas.json') -> None:
        self.sm = serial_manager
        self.n_statistics = n_statistics
        self.n_arduino = n_arduino
        self.err_threshold = err_threshold
        self.n_balanzas = n_balanzas
        self.save_file = save_file

        # calibration
        self.offsets: Optional[UncertaintiesArray] = None
        self.slopes: Optional[UncertaintiesArray] = None

        self.load()

    def load(self) -> bool:
        res = False
        if not os.path.isfile(self.save_file):
            lh.info('Balanzas: No se pudo cargar calibracion porque no existe el archivo')
            return res
        with open(self.save_file, 'r') as f:
            try:
                obj = json.load(f)
                offsets = SmartArray(obj['offsets'])
                slopes = SmartArray(obj['slopes'])
                offsets_error = SmartArray(obj['offsets_error'])
                slopes_error = SmartArray(obj['slopes_error'])

                print(offsets, slopes, offsets_error, slopes_error)

                if len(offsets) == len(slopes) == len(offsets_error) == len(slopes_error) == self.n_balanzas:
                    self.offsets = UncertaintiesArray(offsets, offsets_error)
                    self.slopes = UncertaintiesArray(slopes, slopes_error)
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
        if any(a is None for a in (self.offsets, self.offsets_error, self.slopes, self.slopes_error)):
            lh.warning('Balanzas: No se pudo guardad calibracion porque se calibro aun')
            return False
        obj = {
            'offsets': list(self.offsets.values()),
            'slopes': list(self.slopes.values),
            'offsets_error': list(self.offsets.errors()),
            'slopes_error': list(self.slopes.errors())
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
    
    def read_single_raw(self) -> Optional[SmartArray]:
        res = self.sm.cmd_hx(self.n_arduino)
        if res is None:
            lh.warning(f'Balanzas: No se pudo leer de las balanzas')
            return None
        if len(res) != self.n_balanzas:
            lh.error(f'Balanzas: No se leyo la cantidad correcta de balanzas ({res})')
            return None
        if not all(isinstance(r, float) for r in res):
            lh.error(f'Balanzas: El tipo de dato de las balanzas no es el correcto ({res})')
        return SmartArray(res)
    
    def read_stats_raw(self, n: Optional[int]=None, err_threshold: Optional[float]=None) -> Optional[Tuple[SmartArray, SmartArray, SmartArray, float]]: # [mean, stdev, n_stats_filtered_vals, n_unsuccessful_reads]
        n = self.n_statistics if n is None else n
        err_threshold = self.err_threshold if err_threshold is None else err_threshold
        # si err_threshold <= 0, no se considera, y se toman todos los valores

        vals = list() # [n, self.n_balanzas]
        for _ in trange(n):
            r = self.read_single_raw() # [self.n_balanzas]
            if r:
                vals.append(r)
        n_completed = len(vals)
        n_error = n-n_completed
        vals_t = list(zip(*vals)) # [self.n_balanzas, n]

        means = list(mean(vals_balanza) for vals_balanza in vals_t) #np.mean(vals, axis=0) # should be of size self.n_balanzas
        cleaned_vals = list() # sums, later to be divided. [self.n_balanzas, ?]
        for i in range(self.n_balanzas):
            cleaned_vals.append(list())
            for j in range(n_completed):
                val = vals_t[i][j]
                diff = abs(val - means[i])
                if diff <= err_threshold or err_threshold <= 0:
                    cleaned_vals[-1].append(val)

        if any(len(cv) == 0 for cv in cleaned_vals):
            lh.error(f'Balanzas: cleaned_vals de forma [self.n_balanzas, ?] tiene una sublista de largo nulo')
            return None

        cleaned_means = SmartArray([mean(cv) for cv in cleaned_vals]) # [self.n_balanzas]
        cleaned_stdevs = SmartArray([stdev(cv) for cv in cleaned_vals]) # [self.n_balanzas]
        filtered_vals = SmartArray([n_completed-len(cv) for cv in cleaned_vals])  # [self.n_balanzas]

        lh.debug(f'Balanza: Se leyo las balanzas y hubo {n_error} veces que no se pudo leer del Arduino y {filtered_vals} valores que se descartaron por estadistica')

        return cleaned_means, cleaned_stdevs, filtered_vals, n_error
    
    def read_stats(self, n: Optional[int]=None, err_threshold: Optional[float]=None) -> Optional[Tuple[UncertaintiesArray, SmartArray, float]]: # [mean, stdev, n_stats_filtered_vals, n_unsuccessful_reads]
        if any(a is None for a in (self.offsets, self.slopes)):
            raise Exception('Balanzas have not been calibrated')
        res = self.read_stats_raw(n, err_threshold)
        if res is None:
            return None
        means, stdevs, filtered_vals, unsuccessful_reads = res

        values_raw = UncertaintiesArray(means, stdevs)
        values = (values_raw - self.offsets) / self.slopes

        # means_no_slope = means - self.offsets
        # stdevs_no_slope = sa.sqrt( stdevs**2 + self.offsets_error**2 )

        # means_real = means_no_slope / self.slopes
        # stdevs_real = abs(means_real) * sa.sqrt( (stdevs_no_slope/means_no_slope)**2 + (self.slopes_error/self.slopes)**2 )

        lh.debug(f'Balanza: read_stats -> {values.values()}, {values.errors()}, {filtered_vals}, {unsuccessful_reads}')
        return values, filtered_vals, unsuccessful_reads
    
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

        val = UncertaintiesArray(mean, err)
        self.offsets = val
        return True
    
    def calibrate_slope(self, weights: UncertaintiesArray, n: Optional[int]=None, err_threshold: Optional[float]=None, err_lim: float=1) -> bool:
        n = self.n_statistics*2 if n is None else n
        err_threshold = self.err_threshold if err_threshold is None else err_threshold

        if self.offsets is None:
            lh.warning(f'Balanzas: Can\'t calibrate slope before offset is calibrated')
            return False
        if len(weights) != self.n_balanzas:
            lh.error(f'Balanzas: Couldn\'t calibrate slope since the provided weight list is not of size {self.n_balanzas}. The provided weight list is {weights}')
            return False
        
        if not isinstance(weights, UncertaintiesArray):
            raise TypeError('weights no es una instancia de UncertaintiesArray')

        res = self.read_stats_raw(n, err_threshold)
        if res is None:
            lh.error(f'Balanzas: No se pudo calibrar slope porque read_stats_raw() devolvio None')

        mean, err, _, _ = res
        val = UncertaintiesArray(mean, err)

        # https://en.wikipedia.org/wiki/Propagation_of_uncertainty
        # slope = dy/dx
        # dy = val - self.offsets
        # dy_err = ua.sqrt(err**2 + self.offsets_error**2)
        # slope = dy / weights
        # slope_err = abs(slope) * ua.sqrt( (dy_err/dy)**2 + (weight_errs/weights)**2 )

        slope = (val - self.offsets) / weights

        if any(e > err_lim for e in slope.errors()):
            lh.error(f'Balanzas: Couldn\'t calibrate slope since there were errors greater than the limit {err_lim}. The errors are {slope.errors()}')
            return False

        self.slopes = slope
        return True
        
def calibrate(sm: SerialManager, balanzas: Balanzas, n_balanzas: int, n: int=100):
    sm.cmd_servo_attach(False)

    input('Remove todo el peso de las balanzas y apreta enter')
    print('Tarando...')
    balanzas.calibrate_offset(n, err_threshold=10000, err_lim=10000)
    print(balanzas.offsets, balanzas.offsets_error)
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
            weights = UncertaintiesArray(weights, weights_errs)
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
        print('Offsets: ', balanzas.offsets, 'Offset errs: ', balanzas.offsets_error, 'Slopes', balanzas.slopes, 'Slope errs', balanzas.slopes_error)

if __name__ == '__main__':
    from time import sleep

    sm = SerialManager()

    n_balanzas = 2

    balanzas = Balanzas(sm, n_balanzas, n_statistics=50, n_arduino=20, err_threshold=30)

    balanzas.load()
