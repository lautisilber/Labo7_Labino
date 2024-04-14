import os
from datetime import datetime
from logging_helper import logger as lh
from typing import Optional
from smart_arrays import SmartArrayFloat, SmartArrayBool, SmartArrayInt


class FileManager:
    def __init__(self, n_balanzas: int, save_file: str='data.csv') -> None:
        self.n_balanzas = n_balanzas
        self.save_file = save_file
        self.dirname = os.path.dirname(os.path.abspath(self.save_file))

    def add_entry(self, b_means: SmartArrayFloat, b_stdevs: SmartArrayFloat, pump_states: SmartArrayBool, n_filtered: SmartArrayInt, n_unsuccessful: int, grams_goals: SmartArrayFloat, dht_hum: Optional[float], dht_temp: Optional[float]) -> bool:
        if not os.path.isdir(self.dirname):
            os.makedirs(self.dirname)

        if not (len(b_means) == len(b_stdevs) == len(pump_states) == self.n_balanzas):
            lh.error(f'File Manager: Lengths of arrays do not match or are not {self.n_balanzas}. Lengths are {len(b_means)}, {len(b_stdevs)}, {len(pump_states)}')
            return False

        first_time = not os.path.isfile(self.save_file)
        with open(self.save_file, 'a') as f:
            if first_time:
                f.write('time,')
                for i in range(self.n_balanzas):
                    f.write(f'balanza_avg_{i+1},balanza_std_{i+1},balanza_pump_state_{i+1},n_filtered_{i+1},grams_goals_{i+1},')
                f.write('n_unsuccessful,hum,temp\n')
            now_str = datetime.now().strftime('%Y-%m-%d_%H-%M-%S')
            f.write(now_str + ',')
            for i in range(self.n_balanzas):
                f.write(f'{b_means[i]},{b_stdevs[i]},{1 if pump_states[i] else 0},{n_filtered[i]},{grams_goals[i]},')
            f.write(f'{n_unsuccessful},{dht_hum if dht_hum is not None else ""},{dht_temp if dht_temp is not None else ""}\n')
        
        return True