from typing import Optional, Literal
from random import gauss

n_balanzas = 6

EIGHTBITS = 8
STOPBITS_ONE = 1

class PortNotOpenError(Exception):
    pass

class SerialException(Exception):
    pass

class Serial:
    def __init__(self,
        port: Optional[str]=None,
        baudrate: Optional[int]=None,
        timeout: Optional[int]=None,
        write_timeout: Optional[int]=None,
        bytesize: Optional[int]=None,
        parity: Optional[Literal['N', 'E', 'O']]=None,
        stopbits: Optional[int]=None) -> None:

        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.write_timeout = write_timeout
        self.bytesize = bytesize
        self.parity = parity
        self.stopbits = stopbits

        self._n_balanzas: int = 6
        self._hx_readings: list[float] = [660, 661, 662, 663, 664, 665]
        self._hx_variability_sigma: float = 1.0

        self._is_open: bool = False
        # self._out_buffer: bytearray = bytearray()
        self._in_buffer: bytearray = bytearray()

    @property
    def n_balanzas(self) -> int:
        return self._n_balanzas
    
    @n_balanzas.setter
    def n_balanzas(self, value: int) -> None:
        self._n_balanzas = int(value)

    @property
    def hx_readings(self) -> list[float]:
        return self._hx_readings
    
    @hx_readings.setter
    def hx_readings(self, value: list[float]) -> None:
        if len(value) != self.n_balanzas:
            raise ValueError('Wrong length')
        if not all(isinstance(e, (float, int, bool)) for e in value):
            raise TypeError('Not all elements are numbers')
        self._hx_readings = [float(e) for e in value]

    def hx_reading_change_single(self, value: float, index: int) -> None:
        if index < 0 or index >= self.n_balanzas:
            raise ValueError('Index is wrong')
        self.hx_readings[index] = value

    @property
    def hx_variability_sigma(self) -> float:
        return self._hx_variability_sigma
    
    @hx_variability_sigma.setter
    def hx_variability_sigma(self, value: float) -> None:
        self._hx_variability_sigma = float(value)

    def _get_hx_reading(self, index: Optional[int]=None) -> str:
        r = [gauss(e, self.hx_variability_sigma) for e in self.hx_readings]
        if index is None:
            return str(r)
        if index >= self.n_balanzas:
            raise ValueError()
        return str(r[index])

    def close(self) -> None:
        self._is_open = False

    @property
    def is_open(self) -> bool:
        return self._is_open

    def open(self) -> None:
        self._is_open = True

    @property
    def in_waiting(self) -> int:
        return len(self._in_buffer)

    def _add_to_in_buffer(self, s: str, omit_nl: bool=False) -> None:
        if not omit_nl:
            s += '\n'
        self._in_buffer.extend(s.encode('ascii'))

    def write(self, msg: bytes) -> None:
        if not isinstance(msg, bytes): # type: ignore
            raise TypeError()
        # self._in_buffer.extend(msg)
        s = msg.decode('ascii')
        s = s.replace('\n', '')
        cmd, *args = s.split()
        if cmd == 'hx':
            err = False
            if len(args) > 0:
                err = not args[0].isdecimal()
                if not err:
                    err = not (1 <= int(args[0]) <= 255)
            if not err:
                self._add_to_in_buffer('rcv')
                self._add_to_in_buffer(self._get_hx_reading())
            else:
                self._add_to_in_buffer('ERROR: hx error')
        elif cmd == 'hx_single':
            err = len(args) < 1
            if not err:
                err = not args[0].isdecimal()
                err = not (0 <= int(args[0]) <= n_balanzas)
            if len(args) > 1:
                err = err or not args[1].isdecimal()
                if not err:
                    err = not (1 <= int(args[1]) <= 255)
            if not err:
                self._add_to_in_buffer('rcv')
                self._add_to_in_buffer(self._get_hx_reading(int(args[0])))
            else:
                self._add_to_in_buffer('ERROR: hx_single error')
        elif cmd == 'hx_n':
            self._add_to_in_buffer(str(n_balanzas))
        elif cmd == 'dht':
            self._add_to_in_buffer('{"hum":12.34,"temp":56.78}')
        elif cmd == 'stepper':
            err = len(args) < 1
            if not err:
                if len(args[0]) > 1:
                    err = not (args[0].isdecimal() or (args[0][0] == '-' and args[0][1:].isdecimal()))
                else:
                    err = not args[0].isdecimal()
                err = err or not (args[1] in ('0', '1', 'true', 'false', 'True', 'False', 'TRUE', 'FALSE'))
            if not err:
                self._add_to_in_buffer('rcv')
                self._add_to_in_buffer('OK')
            else:
                self._add_to_in_buffer('ERROR: stepper error')
        elif cmd == 'servo':
            err = len(args) < 1
            if not err:
                err = not args[0].isdecimal()
                if not err:
                    err = not (1 <= int(args[0]) <= 179)
            if not err:
                self._add_to_in_buffer('rcv')
                self._add_to_in_buffer(str(int(args[0])))
            else:
                self._add_to_in_buffer('ERROR: servo error')
        elif cmd == 'pump':
            err = len(args) < 2
            if not err:
                err = not args[0].isdecimal() and not args[0].isdecimal()
                if not err:
                    err = int(args[0]) < 0 or not (0 <= int(args[1]) <= 255)
            if not err:
                self._add_to_in_buffer('rcv')
                self._add_to_in_buffer('OK')
            else:
                self._add_to_in_buffer('ERROR: pump error')
        elif cmd == 'stepper_attach':
            err = len(args) < 1
            if not err:
                err = args[0] not in ('0', '1', 'true', 'false', 'True', 'False', 'TRUE', 'FALSE')
            if not err:
                self._add_to_in_buffer('OK')
            else:
                self._add_to_in_buffer('ERROR: stepper_attach error')
        elif cmd == 'servo_attach':
            err = len(args) < 1
            if not err:
                err = args[0] not in ('0', '1', 'true', 'false', 'True', 'False', 'TRUE', 'FALSE')
            if not err:
                self._add_to_in_buffer('OK')
            else:
                self._add_to_in_buffer('ERROR: servo_attach error')
        elif cmd == 'ok':
            self._add_to_in_buffer('OK')
        else:
            self._add_to_in_buffer(f'ERROR: unknown command {s}')


    def read(self, size: int=1) -> bytes:
        if not self.in_waiting: return bytes()
        if size >= self.in_waiting:
            r = bytes(self._in_buffer)
            self._in_buffer = bytearray()
        else:
            r = bytes(self._in_buffer[:size])
            self._in_buffer = self._in_buffer[size:]
        return r

    def read_until(self, expected: bytes=b'\n', size: Optional[int]=None) -> bytes:
        if size is None:
            size = self.in_waiting
        r = bytearray()
        byte: Optional[bytes] = None
        i: int = 0
        while byte != expected and i < size:
            byte = self.read()
            if byte != expected:
                r.extend(byte)
        return bytes(r)

    def reset_input_buffer(self) -> None:
        self._in_buffer = bytearray()