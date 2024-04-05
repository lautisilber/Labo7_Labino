from typing import Optional, Literal

n_balanzas = 6

EIGHTBITS = 8
STOPBITS_ONE = 1

class PortNotOpenError(Exception):
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

        self._is_open: bool = False
        # self._out_buffer: bytearray = bytearray()
        self._in_buffer: bytearray = bytearray()

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
        if not isinstance(msg, bytes):
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
                self._add_to_in_buffer('[1.2,3.4,5.6,7.8,9.1,2.3]')
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
                self._add_to_in_buffer('[1.2,3.4,5.6,7.8,9.1,2.3]')
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
        return r

    def reset_input_buffer(self) -> None:
        self._in_buffer = bytearray()