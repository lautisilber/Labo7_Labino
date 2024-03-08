from __future__ import annotations
from typing import Iterable, Optional, Literal, Union, List, Tuple, Callable
import operator
import math
from . import _generic_operations as go
from ._utils import castable, calculate_dominant_type, calculate_dominant_type_from_iter

scalar_t = Union[bool, int, float, complex]

def _cdt(a: Union[SmartArray, SmartList, scalar_t], b: Union[SmartArray, SmartList, scalar_t]) -> type:
    if isinstance(a, (SmartArray, SmartList)):
        t1 = a.dtype
    elif isinstance(a, scalar_t):
        t1 = type(a)
    else:
        raise TypeError()
    if isinstance(b, (SmartArray, SmartList)):
        t2 = b.dtype
    elif isinstance(b, scalar_t):
        t2 = type(b)
    else:
        raise TypeError()
    return calculate_dominant_type(t1, t2)

def filled(size: int, value: Union[scalar_t], dtype: Optional[scalar_t]=None) -> SmartArray:
    if size < 0:
        raise ValueError()
    return SmartArray([value]*size, dtype=dtype)

def zeros(size: int, dtype: Optional[scalar_t]=None) -> SmartArray:
    return filled(size, 0.0, dtype)

def sqrt(a: SmartArray) -> SmartArray:
    return SmartArray(go._generic_unary_op(a, math.sqrt))

class SmartArray:
    def __init__(self, a: Optional[Union[Iterable, SmartArray, SmartList]]=None, dtype: Optional[type]=None) -> None:
        if isinstance(a, (SmartArray, SmartList)):
            self.arr = a.arr.copy()
            self.t = a.dtype
        else:
            if not isinstance(a, Iterable):
                raise TypeError()
            if dtype is None:
                self.t = calculate_dominant_type_from_iter(a)
            else:
                if not issubclass(dtype, scalar_t):
                    raise ValueError(f'dtype {dtype} is not allowed')
                self.t = dtype
            if not issubclass(self.t, scalar_t):
                raise TypeError('not all elements of the iterable are scalar')
            if not all(castable(e, self.t) for e in a):
                raise TypeError('not all elements of the iterable are of same type')
            self.arr = [self.t(e) for e in a]

    @property
    def size(self) -> int:
        return len(self.arr)
    
    @property
    def dtype(self) -> type:
        return self.t
    
    def __len__(self) -> int:
        return self.size
    
    def __iter__(self):
        return iter(self.arr)
    
    def __getitem__(self, key: int) -> scalar_t:
        return self.arr[key]
    
    def __setitem__(self, key: int, value: scalar_t):
        if not isinstance(value, self.t) and not castable(value, self.t):
            raise TypeError(f'value is not of type {self.t}')
        self.arr[key] = self.t(value)

    def bool(self) -> SmartArray:
        return SmartArray((bool(e) for e in self), dtype=bool)
    
    def int(self) -> SmartArray:
        return SmartArray((int(e) for e in self), dtype=int)
    
    def float(self) -> SmartArray:
        return SmartArray((float(e) for e in self), dtype=float)
    
    def complex(self) -> SmartArray:
        return SmartArray((complex(e) for e in self), dtype=complex)

    def reverse(self) -> None:
        self.arr.reverse()

    def sort(self, *args, key: Optional[Callable]=None, reverse: bool=False) -> None:
        self.arr.sort(*args, key=key, reverse=reverse)

    def copy(self) -> SmartArray:
        return SmartArray(self)
    
    # math ops

    def __add__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_op(self, other, operator.add, scalar_t), dtype=_cdt(self, other))
    
    def __sub__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_op(self, other, operator.sub, scalar_t), dtype=_cdt(self, other))
    
    def __mul__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_op(self, other, operator.mul, scalar_t), dtype=_cdt(self, other))
    
    def __truediv__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_op(self, other, operator.truediv, scalar_t), dtype=_cdt(self, other))
    
    def __floordiv__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_op(self, other, operator.floordiv, scalar_t), dtype=_cdt(self, other))
    
    def __pow__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_op(self, other, operator.pow, scalar_t), dtype=_cdt(self, other))
    
    def __mod__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_op(self, other, operator.mod, scalar_t), dtype=_cdt(self, other))
    
    def __radd__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return self.__add__(other)
    
    def __rsub__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_op_rightsided(other, self, operator.sub, scalar_t), dtype=_cdt(self, other))

    def __rmul__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return self.__mul__(other)
    
    def __rtruediv__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_op_rightsided(other, self, operator.truediv, scalar_t), dtype=_cdt(self, other))
    
    def __rfloordiv__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_op_rightsided(other, self, operator.floordiv, scalar_t), dtype=_cdt(self, other))
    
    def __rmod__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_op_rightsided(other, self, operator.fmod, scalar_t), dtype=_cdt(self, other))
    
    # unary ops

    def __abs__(self) -> SmartArray:
        return SmartArray(go._generic_unary_op(self.arr, abs), dtype=self.t)
    
    def __pos__(self) -> SmartArray:
        return SmartArray(go._generic_unary_op(self.arr, operator.pos), dtype=self.t)
    
    def __neg__(self) -> SmartArray:
        return SmartArray(go._generic_unary_op(self.arr, operator.neg), dtype=self.t)
    
    def __invert__(self) -> SmartArray:
        return SmartArray(go._generic_unary_op(self.arr, operator.invert), dtype=self.t)
    
    def __ceil__(self) -> SmartArray:
        return SmartArray(go._generic_unary_op(self.arr, operator.ceil), dtype=bool if self.t is bool else int)

    def __floor__(self) -> SmartArray:
        return SmartArray(go._generic_unary_op(self.arr, operator.floor), dtype=bool if self.t is bool else int)

    def __trunc__(self) -> SmartArray:
        return SmartArray(go._generic_unary_op(self.arr, operator.trunc), dtype=bool if self.t is bool else int)

    # bool ops

    def __eq__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_logic_op(self, other, operator.eq, scalar_t), dtype=bool)
    
    def __ne__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_logic_op(self, other, operator.ne, scalar_t), dtype=bool)
    
    def __lt__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_logic_op(self, other, operator.lt, scalar_t), dtype=bool)
    
    def __gt__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_logic_op(self, other, operator.gt, scalar_t), dtype=bool)
    
    def __le__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_logic_op(self, other, operator.le, scalar_t), dtype=bool)
    
    def __ge__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_logic_op(self, other, operator.ge, scalar_t), dtype=bool)
    
    def __req__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_logic_op_rightsided(other, self, operator.eq, scalar_t), dtype=bool)
    
    def __rne__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_logic_op_rightsided(other, self, operator.ne, scalar_t), dtype=bool)
    
    def __rlt__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_logic_op_rightsided(other, self, operator.lt, scalar_t), dtype=bool)
    
    def __rgt__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_logic_op_rightsided(other, self, operator.gt), dtype=bool)
    
    def __rle__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_logic_op_rightsided(other, self, operator.le), dtype=bool)
    
    def __rge__(self, other: Union[SmartArray, SmartList, scalar_t]) -> SmartArray:
        return SmartArray(go._generic_binary_logic_op_rightsided(other, self, operator.ge), dtype=bool)

    # utils

    def __list__(self) -> List:
        return self.arr.tolist()

    def __repr__(self) -> str:
        return f'SmartArray({list(self)})'
    
    def __str__(self) -> str:
        return self.__repr__()
    
class SmartList(SmartArray):
    def __init__(self, a: Optional[Union[Iterable, SmartArray, SmartList]]=None) -> None:
        super().__init__(a)

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