from __future__ import annotations
from typing import Iterable, Optional, Literal, Union, List, Tuple, Callable, Literal
import operator as op
import math
from . import _generic_operations as go
from ._utils import castable, calculate_dominant_type, calculate_dominant_type_from_iter
import os, json
from typing import TypeVar, Generic
from warnings import warn

scalar_t = Union[bool, int, float, complex]
scalar_type_list = (bool, int, float, complex)
scalar_type_dominance = {bool: 0, int: 1, float: 2, complex: 3}
T = TypeVar('T')
U = TypeVar('U')
V = TypeVar('V')

def _get_dominant_type(t1: scalar_t, t2: scalar_t) -> bool:
    return scalar_type_dominance[t1] >= scalar_type_dominance[t2]

def filled(size: int, value: T) -> SmartArray[T]:
    if size < 0:
        raise ValueError()
    if not isinstance(value, scalar_type_list):
        raise TypeError(f'value is of type {type(value)} which is not supported')
    return SmartArray[T]([value]*size)

def zeros(size: int, dtype: Optional[T]=None) -> SmartArray[T]:
    if not any(e is dtype for e in scalar_type_list):
        raise TypeError(f'dtype is type {dtype} which is not supported')
    return filled(size, dtype(0))

def sqrt(a: SmartArray[T]) -> SmartArray[T]:
    return SmartArray[T](go._generic_unary_op(a, math.sqrt))

def dump_dict(array: Union[SmartArray, SmartList]) -> dict[str,Union[str,List[Union[int,float,List[float]]]]]:
    if isinstance(array, SmartArray):
        kind = 'smart_array'
    elif isinstance(array, SmartList):
        kind = 'smart_list'
    else:
        raise TypeError(f'array is not a SmartArray nor a SmartList. It is instead of type {type(array)}')
    dtype = float
    for t, n in zip(scalar_type_list, ('bool', 'int', 'float', 'complex')):
        if array.dtype is t:
            dtype = n
            break
    if dtype != 'complex':
        l = list(array)
    else:
        l = [[e.real, e.imag] for e in array]
    obj = {
        'library': 'smart_array',
        'kind': kind,
        'dtype': dtype,
        'array': l
    }
    return obj

def dump(array: Union[SmartArray, SmartList], save_file: str) -> None:
    obj = dump_dict(array)
    d = os.path.dirname(save_file)
    if d:
        if not os.path.isdir(d):
            os.makedirs(d)
    with open(save_file, 'w') as f:
        json.dump(obj, f)

def load_dict(obj: dict[str,Union[str,List[Union[int,float,List[float]]]]]) -> Union[SmartArray, SmartList]:
    if not obj['library'] == 'smart_array': raise ValueError()
    if obj['dtype'] == 'bool': dtype = bool
    elif obj['dtype'] == 'int': dtype = int
    elif obj['dtype'] == 'float': dtype = float
    elif obj['dtype'] == 'complex': dtype = complex
    else: raise ValueError()
    if not isinstance(obj['array'], Iterable): raise ValueError()
    if not (dtype is complex):
        l = tuple(type(e) for e in obj['array'])
    else:
        l = tuple(complex(e[0], e[1]) for e in obj['array'])
    if obj['kind'] == 'smart_array':
        return SmartArray(l, dtype=dtype)
    elif obj['kind'] == 'smart_list':
        return SmartList(l, dtype=dtype)
    else:
        raise ValueError()

def load(save_file: str) -> Union[SmartArray, SmartList]:
    with open(save_file, 'r') as f:
        obj = json.load(f)
    return load_dict(obj)

class SmartArray(Generic[T]):
    def __init__(self, a: Optional[Union[Iterable, SmartArray, SmartList]]=None) -> None:
        # if isinstance(a, (SmartArray, SmartList)):
        #     self.arr = a.arr.copy()
        #     self.t = a.dtype
        # else:
        #     if not isinstance(a, Iterable):
        #         raise TypeError()
        #     if isinstance(a, Generator):
        #         a = tuple(a)
        #     if dtype is None:
        #         self.t = calculate_dominant_type_from_iter(a)
        #     else:
        #         if not any(dtype is t for t in scalar_type_list):
        #             raise ValueError(f'dtype {dtype} is not allowed')
        #         self.t = dtype
        #     if not any(self.t is t for t in scalar_type_list):
        #         raise TypeError('not all elements of the iterable are scalar')
        #     if not all(castable(e, self.t) for e in a):
        #         raise TypeError('not all elements of the iterable are of same type')
        #     self.arr = [self.t(e) for e in a]
        if isinstance(a, (SmartArray, SmartList)):
            self.arr: list[T] = a.arr.copy()
            self.t: T = a.t
        else:
            if a is None:
                self.arr: list[T] = list()
                self.t: T = float
                warn('Initialized with empty Iterable. self.t defaulted to float. If this is not the typing you want, you have to change it manually')
            else:
                self.arr: list[T] = list(a)
            if len(self.arr) > 1:
                if not all(type(e) == type[self.arr[0]] for e in self.arr[1:]):
                    raise TypeError('Not all elements of a are of same type')
                if not all(isinstance(e, scalar_type_list) for e in self.arr):
                    raise TypeError('a contains elements of unsupported typing. Only types supported are bool, int, float and complex')
                self.t: T = type(self.arr[0])

    @property
    def size(self) -> int:
        return len(self.arr)

    @property
    def dtype(self) -> T:
        return self.__orig_class__.__args__[0]

    def __len__(self) -> int:
        return self.size

    def __iter__(self):
        return iter(self.arr)

    def __getitem__(self, key: int) -> T:
        return self.arr[key]

    def __setitem__(self, key: int, value: scalar_t) -> None:
        if not isinstance(value, self.t) and not castable(value, self.t):
            raise TypeError(f'value is not of type {self.t}')
        self.arr[key] = self.t(value)

    def bool(self) -> SmartArray[bool]:
        if self.t is complex:
            return SmartArray[bool](bool(e.real) for e in self)
        else:
            return SmartArray[bool](bool(e) for e in self)

    def int(self) -> SmartArray[int]:
        if self.t is complex:
            return SmartArray[int](int(e.real) for e in self)
        else:
            return SmartArray[int](int(e) for e in self)

    def float(self) -> SmartArray[float]:
        if self.t is complex:
            return SmartArray[float](float(e.real) for e in self)
        else:
            return SmartArray[float](float(e) for e in self)

    def complex(self) -> SmartArray[complex]:
        if self.t is complex:
            return SmartArray[complex](self)
        else:
            return SmartArray[complex](complex(e, 0) for e in self)

    def reverse(self) -> None:
        self.arr.reverse()

    def sort(self, *args, key: Optional[Callable]=None, reverse: bool=False) -> None:
        self.arr.sort(*args, key=key, reverse=reverse)

    def copy(self) -> SmartArray[T]:
        return SmartArray[T](self)

    def dump(self, save_file: str) -> None:
        dump(self, save_file)

    # math ops
        
    def _binary_op(self, b: Union[Iterable[T], T], op: Callable[[T,T],T]) -> Tuple[T,...]:
        if isinstance(b, Iterable):
            if not len(a) == len(b):
                raise IndexError('Arrays are not of same size')
            if not all(isinstance(e, a.t) for e in b):
                raise TypeError(f'other is not of type {self.t}. other is of type {type(b[0])}')
            return tuple(op(e1, e2) for e1, e2 in zip(a, b))
        elif isinstance(b, self.t):
            return tuple(op(e,b) for e in a)
        else:
            raise TypeError(f'other is not of type {self.t}. other is of type {type(b)}')

    def __add__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[T]:
        return self.__class__[T](self._binary_op(other, op.add))

    def __sub__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[T]:
        return self.__class__[T](self._binary_op(other, op.sub))

    def __mul__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[T]:
        return self.__class__[T](self._binary_op(other, op.mul))

    def __truediv__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[float]:
        return self.__class__[int](self._binary_op(other, op.truediv))

    def __floordiv__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[T]:
        return SmartArray(go._generic_binary_op(self, other, op.floordiv, scalar_type_list))

    def __pow__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[T]:
        return SmartArray(go._generic_binary_op(self, other, op.pow, scalar_type_list))

    def __mod__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[T]:
        return SmartArray(go._generic_binary_op(self, other, op.mod, scalar_type_list))

    def __radd__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[T]:
        return self.__add__(other)

    def __rsub__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[T]:
        return SmartArray(go._generic_binary_op_rightsided(other, self, op.sub, scalar_type_list))

    def __rmul__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[T]:
        return self.__mul__(other)

    def __rtruediv__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[T]:
        return SmartArray(go._generic_binary_op_rightsided(other, self, op.truediv, scalar_type_list))

    def __rfloordiv__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[T]:
        return SmartArray(go._generic_binary_op_rightsided(other, self, op.floordiv, scalar_type_list))

    def __rmod__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[T]:
        return SmartArray(go._generic_binary_op_rightsided(other, self, op.fmod, scalar_type_list))

    # unary ops

    def __abs__(self) -> SmartArray[T]:
        return SmartArray(go._generic_unary_op(self.arr, abs))

    def __pos__(self) -> SmartArray[T]:
        return SmartArray(go._generic_unary_op(self.arr, op.pos))

    def __neg__(self) -> SmartArray[T]:
        return SmartArray(go._generic_unary_op(self.arr, op.neg))

    def __invert__(self) -> SmartArray[T]:
        return SmartArray(go._generic_unary_op(self.arr, op.invert))

    def __ceil__(self) -> SmartArray[int]:
        return SmartArray(go._generic_unary_op(self.arr, op.ceil), dtype=bool if self.t is bool else int)

    def __floor__(self) -> SmartArray[int]:
        return SmartArray(go._generic_unary_op(self.arr, op.floor), dtype=bool if self.t is bool else int)

    def __trunc__(self) -> SmartArray[int]:
        return SmartArray(go._generic_unary_op(self.arr, op.trunc), dtype=bool if self.t is bool else int)

    # bool ops

    def __eq__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[bool]:
        return SmartArray(go._generic_binary_logic_op(self, other, op.eq, scalar_type_list), dtype=bool)

    def __ne__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[bool]:
        return SmartArray(go._generic_binary_logic_op(self, other, op.ne, scalar_type_list), dtype=bool)

    def __lt__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[bool]:
        return SmartArray(go._generic_binary_logic_op(self, other, op.lt, scalar_type_list), dtype=bool)

    def __gt__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[bool]:
        return SmartArray(go._generic_binary_logic_op(self, other, op.gt, scalar_type_list), dtype=bool)

    def __le__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[bool]:
        return SmartArray(go._generic_binary_logic_op(self, other, op.le, scalar_type_list), dtype=bool)

    def __ge__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[bool]:
        return SmartArray(go._generic_binary_logic_op(self, other, op.ge, scalar_type_list), dtype=bool)

    def __req__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[bool]:
        return SmartArray(go._generic_binary_logic_op_rightsided(other, self, op.eq, scalar_type_list), dtype=bool)

    def __rne__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[bool]:
        return SmartArray(go._generic_binary_logic_op_rightsided(other, self, op.ne, scalar_type_list), dtype=bool)

    def __rlt__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[bool]:
        return SmartArray(go._generic_binary_logic_op_rightsided(other, self, op.lt, scalar_type_list), dtype=bool)

    def __rgt__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[bool]:
        return SmartArray(go._generic_binary_logic_op_rightsided(other, self, op.gt), dtype=bool)

    def __rle__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[bool]:
        return SmartArray(go._generic_binary_logic_op_rightsided(other, self, op.le), dtype=bool)

    def __rge__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray[bool]:
        return SmartArray(go._generic_binary_logic_op_rightsided(other, self, op.ge), dtype=bool)

    # utils

    def __list__(self) -> List:
        return list(self.arr)

    def __repr__(self) -> str:
        return f'SmartArray({list(self)})'

    def __str__(self) -> str:
        return self.__repr__()

class SmartList(SmartArray):
    def __init__(self, a: Optional[Union[Iterable, SmartArray, SmartList]]=None, dtype: Optional[scalar_t]=None) -> None:
        super().__init__(a, dtype)

    def copy(self) -> SmartList:
        return SmartList(self)

    def bool(self) -> SmartList:
        return SmartList((bool(e) for e in self), dtype=bool)

    def int(self) -> SmartList:
        return SmartList((int(e) for e in self), dtype=int)

    def float(self) -> SmartList:
        return SmartList((float(e) for e in self), dtype=float)

    def complex(self) -> SmartList:
        return SmartList((complex(e) for e in self), dtype=complex)

    def append(self, x: scalar_t) -> None:
        if not isinstance(x, scalar_t):
            raise TypeError()
        self.arr.append(x)

    def extend(self, it: Iterable) -> None:
        if not all(isinstance(e, scalar_t) for e in it):
            raise TypeError()
        self.arr.extend(it)

    def clear(self) -> None:
        self.arr.clear()

    def insert(self, i: int, x: scalar_t) -> None:
        if not isinstance(x, scalar_t):
            raise TypeError()
        self.arr.index(i, x)

    def pop(self, i: int) -> scalar_t:
        return self.arr.pop(i)

    def __repr__(self) -> str:
        return f'SmartList({list(self)})'


if __name__ == '__main__':
    from uncertainties import ufloat
    a = SmartList([1, 2, 3])
    b = SmartArray([4, 5, 6])


    c = a.copy()
    c[0] = 7
    b /= a % c ** 3 + 18

    print(a, c, b)
