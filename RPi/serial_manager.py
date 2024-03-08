import serial
from time import sleep, time
from typing import List, Optional, Tuple, Any, Union, Literal
import sys
import glob
import json
from logging_helper import logger as lh


def get_devices() -> List[str]:
    """ Lists serial port names

        :raises EnvironmentError:
            On unsupported or unknown platforms
        :returns:
            A list of the serial ports available on the system
    """
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

def try_cast_omit_none(v: Any, t: type) -> Optional[Any]:
    if v is None:
        return None
    try:
        v = t(v)
    except:
        v = None
    return v

class SerialManagerGeneric:
    END_CHAR = b'\n'
    SEP_CHAR = b' '
    
    def __init__(self, port: str='/dev/ttyS0', baud_rate: int=9600, timeout: int=5, delay_s: int=.15, n_retries: int=3) -> None:
        devices = get_devices()
        if port not in devices:
            raise Exception(f'Port "{port}" is not among de available devices: {devices}')
        self.port = port
        self.baud_rate = baud_rate
        self.timeout = timeout
        self.delay_s = delay_s
        self.n_retries = n_retries

        self.serial = serial.Serial()
        self.serial.port = self.port
        self.serial.baudrate = self.baud_rate
        self.serial.timeout = self.timeout
        self.serial.write_timeout = self.timeout
        self.serial.bytesize = serial.EIGHTBITS
        self.serial.parity = serial.PARITY_NONE
        self.serial.stopbits = serial.STOPBITS_ONE # probably

    def close(self, log: bool=True) -> None:
        if self.is_open():
            if log:
                lh.info('SM: Closing serial port')
            self.serial.close()

    def __del__(self) -> None:
        # we can't log in this instance because of an issue with the
        # logging library. A method (open) is garbage collected before
        # the logging and it creates an exception
        # https://stackoverflow.com/questions/64679139/nameerror-name-open-is-not-defined-when-trying-to-log-to-files
        self.close(False)

    def is_open(self) -> bool:
        return self.serial.is_open

    def flush(self) -> None:
        self.serial.reset_input_buffer()

    def open(self) -> None:
        if not self.is_open():
            self.close()
        self.serial.open()
        self.flush()
        lh.info('Serial: Opening serial port')
        sleep(self.delay_s)

    def write(self, command: str) -> None:
        self.serial.write(command.encode('utf-8') + SerialManager.END_CHAR)
        lh.debug(f'Serial write: "{command}"')

    def read(self) -> Optional[str]:
        res = self.serial.read_until(SerialManager.END_CHAR)
        lh.debug(f'Serial read: "{res}"')
        if not res:
            return None
        res = res.decode('utf-8').rstrip()
        return res
    
class SerialManager(SerialManagerGeneric):
    RCV_STR = 'rcv'
    def __init__(self, port: str='/dev/ttyS0', baud_rate: int=9600, timeout: int=5, delay_s: int=0.1) -> None:
        super().__init__(port, baud_rate, timeout, delay_s)
        self._hx_n_init_time_s = 0
        self._hx_n_total_time_s = 30
        self._last_hx_n: Optional[int] = None

    def _send_command_wait_response(self, command: str, timeout_long_s: int = 30, use_delay: bool = True) -> Optional[str]:
        if not self.is_open():
            raise serial.PortNotOpenError()
        if not command:
            return None
        
        self.flush()
        self.write(command)
        if use_delay:
            sleep(self.delay_s)
        res = self.read()
        if res == SerialManager.RCV_STR:
            if timeout_long_s > 0:
                start_time = time()
                while (not self.serial.in_waiting) and (abs(time() - start_time) < timeout_long_s):
                    sleep(.05)
            res = self.read()

        if not res:
            return None
        if 'ERROR' in res:
            lh.warning(f'Arduino error: "{res}"')
            return None
        if use_delay:
            sleep(self.delay_s)
        return res

    def _send_command_wait_response_retries(self, command: str, timeout_long_s: int=30, use_delay: bool=True, n_retries: Optional[int]=None) -> Optional[str]:
        n_retries = max(self.n_retries if n_retries is None else n_retries, 1)
        for _ in range(n_retries):
            res = self._send_command_wait_response(command, timeout_long_s, use_delay)
            if res is not None:
                return res
        return None

    def cmd_ok(self) -> bool:
        res = self._send_command_wait_response_retries('ok')
        res = res == 'OK'
        if not res:
            lh.warning('Arduino: Failed OK command')
        else:
            lh.debug('Arduino: OK command successful')
        return res
    
    def cmd_hx(self, n: int=20) -> Optional[List[float]]:
        '''
            n son la cantidad de veces que se samplean las balanzas para obtener el promedio
        '''
        if not isinstance(n, int):
            raise TypeError()
        if n < 0:
            raise ValueError()
        res = self._send_command_wait_response_retries(f'hx {n}')
        if res:
            try:
                res = json.loads(res)
            except:
                res = None
        if res is None:
            lh.warning('Arduino: Failed hx command')
        else:
            lh.debug(f'Arduino: Succeeded hx ({res})')
        return res
    
    def cmd_hx_n(self) -> Optional[int]:
        '''
            devuelve la cantidad de balanzas
        '''
        if (self._last_hx_n is None) or ((time() - self._hx_n_init_time_s) > self._hx_n_total_time_s):
            res = self._send_command_wait_response_retries('hx_n')
            res = try_cast_omit_none(res, int)
            if res is None:
                lh.warning('Arduino: Failed hx_n command')
                return None
            self._hx_n_init_time_s = time()
            self._last_hx_n = res
            lh.debug(f'Arduino: Succeeded hx_n command with {res}')
            return res
        else:
            lh.debug(f'Arduino: Used cached hx_n value of {self._last_hx_n}')
            return self._last_hx_n
    
    def cmd_dht(self) -> Optional[Tuple[float, float]]: # hum, temp
        res = self._send_command_wait_response_retries('dht')
        if res:
            try:
                res = json.loads(res)
                res = res['hum'], res['temp']
            except:
                res = None
        if res is None:
            lh.warning('Arduino: Failed dht command')
        else:
            lh.debug(f'Arduino: Succeeded dht command with {res}')
        return res
    
    def cmd_stepper(self, posicion: Optional[int]=None) -> Optional[int]:
        '''
            si posicion es un numero, es el indice de la posicion a la que se  el stepper.
            devuelve la posicion en la que se encuentra el stepper al final
        '''
        if not (isinstance(posicion, int) or posicion is None):
            raise TypeError()
        if posicion < 0:
            raise ValueError()
        cmd = 'stepper' if posicion is None else f'stepper {posicion}'
        res = self._send_command_wait_response_retries(cmd)
        res = try_cast_omit_none(res, int)
        if res is None:
            lh.warning('Arduino: Failed stepper command')
        else:
            lh.debug(f'Arduino: Succeeded stepper command with {res}')
        return res
    
    def cmd_servo(self, angulo: Optional[int]=None) -> Optional[int]:
        '''
            si angulo es un numero, es el angulo al que se llevara el stepper.
            devuelve el angulo en la que se encuentra el stepper al final
        '''
        if not (isinstance(angulo, int) or angulo is None):
            raise TypeError()
        if angulo < 1 or angulo > 179:
            raise ValueError()
        cmd = 'servo' if angulo is None else f'servo {angulo}'
        res = self._send_command_wait_response_retries(cmd)
        res = try_cast_omit_none(res, int)
        if res is None:
            lh.warning('Arduino: Failed servo command')
        else:
            lh.debug(f'Arduino: Succeeded servo command with {res}')
        return res
    
    def cmd_pump(self, tiempo: int, intensidad: int) -> bool:
        '''
            prende la bomba por el tiempo en ms indicado, en la intensidad en % indicada
        '''
        if not all(isinstance(v, int) for v in (tiempo, intensidad)):
            raise TypeError()
        if tiempo <= 0 or (intensidad <= 0 or intensidad > 100):
            raise ValueError()
        res = self._send_command_wait_response_retries(f'pump {tiempo} {intensidad}')
        res = res == 'OK'
        if res is None:
            lh.warning('Arduino: Failed pump command')
        else:
            lh.debug(f'Arduino: Succeeded pump command')
        return res
    
    def cmd_stepper_attach(self, attach: Union[bool, Literal['true'], Literal['True'], Literal['TRUE'],
                                             Literal['false'], Literal['False'], Literal['FALSE']]):
        if not (isinstance(attach, bool) or any(attach == b for b in ('true', 'True', 'TRUE', 'false', 'False', 'FALSE'))):
            raise TypeError()
        res = self._send_command_wait_response_retries(f'stepper_attach {attach}')
        if res is None:
            lh.warning('Arduino: Failed stepper_attach command')
        else:
            lh.debug(f'Arduino: Succeeded stepper_attach command with {res}')
        return res
    
    def cmd_servo_attach(self, attach: Union[bool, Literal['true'], Literal['True'], Literal['TRUE'],
                                             Literal['false'], Literal['False'], Literal['FALSE']]):
        if not (isinstance(attach, bool) or any(attach == b for b in ('true', 'True', 'TRUE', 'false', 'False', 'FALSE'))):
            raise TypeError()
        res = self._send_command_wait_response_retries(f'servo_attach {attach}')
        if res is None:
            lh.warning('Arduino: Failed servo_attach command')
        else:
            lh.debug(f'Arduino: Succeeded servo_attach command with {res}')
        return res
    
if __name__ == '__main__':
    sm = SerialManager()
    sm.open()
    print(sm.cmd_ok())
    print(sm.cmd_hx())
