from __future__ import annotations
from typing import Iterable, Optional, Literal, Union, List, Tuple, Callable, Generator
import operator
import math
from uncertainties import umath
from uncertainties.core import AffineScalarFunc as ufloat_t
from uncertainties import ufloat
from . import _generic_operations as go
from ._utils import castable
from .smart_array import SmartArray, SmartList



def filled(size: int, value: Union[ufloat_t]) -> UncertaintiesArray:
    if size < 0:
        raise ValueError()
    return UncertaintiesArray([value]*size)

def zeros(size: int) -> UncertaintiesArray:
    return filled(size, 0)

def sqrt(a: UncertaintiesArray) -> UncertaintiesArray:
    return UncertaintiesArray(go._generic_unary_op(a, math.sqrt))

class UncertaintiesArray:
    def __init__(self, a: Optional[Union[Iterable, UncertaintiesArray, UncertaintiesList]]=None, b: Optional[Iterable]=None) -> None:
        if not isinstance(a, Iterable):
            raise TypeError('a is not iterable')
        if isinstance(a, Generator):
            a = tuple(a)
        if b is not None:
            if not isinstance(b, Iterable):
                raise TypeError('b is not iterable')
            if isinstance(b, Generator):
                b = tuple(b)
            if len(a) != len(b):
                raise IndexError('a and b are not of same size')
            if not (all(castable(e, float) for e in a) and all(castable(e, float) for e in b)):
                raise TypeError('not all elements of a and b are castable to float')
            a = list(ufloat(e1, e2) for e1, e2 in zip(a, b))
        if isinstance(a, UncertaintiesArray):
            self.arr = a.arr.copy()
        else:
            if not all(isinstance(e, ufloat_t) for e in a):
                raise TypeError('not all elements of the iterable are of type ufloat_t')
            if isinstance(a, list):
                self.arr = a.copy()
            else:
                self.arr = list(a)

    @property
    def size(self) -> int:
        return len(self.arr)
    
    def __len__(self) -> int:
        return self.size
    
    def __iter__(self):
        return iter(self.arr)
    
    def __getitem__(self, key: int) -> ufloat_t:
        return self.arr[key]
    
    def __setitem__(self, key: int, value: ufloat_t):
        if not isinstance(value, ufloat_t):
            raise TypeError()
        self.arr[key] = value

    def reverse(self) -> None:
        self.arr.reverse()

    def sort(self, *args, key: Optional[Callable]=None, reverse: bool=False) -> None:
        self.arr.sort(*args, key=key, reverse=reverse)

    def copy(self) -> UncertaintiesArray:
        return UncertaintiesArray(self)
    
    def values(self) -> SmartArray:
        return SmartArray(tuple(e.nominal_value for e in self), dtype=float)
    
    def errors(self) -> SmartArray:
        return SmartArray(tuple(e.std_dev for e in self), dtype=float)
    
    # math ops

    def __add__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_op(self, other, operator.add, ufloat_t))
    
    def __sub__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_op(self, other, operator.sub, ufloat_t))
    
    def __mul__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_op(self, other, operator.mul, ufloat_t))
    
    def __truediv__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_op(self, other, operator.truediv, ufloat_t))
    
    def __floordiv__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_op(self, other, operator.floordiv, ufloat_t))
    
    def __pow__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_op(self, other, umath.pow, ufloat_t))
    
    def __mod__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_op(self, other, umath.fmod, ufloat_t))
    
    def __radd__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return self.__add__(other)
    
    def __rsub__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_op_rightsided(other, self, operator.sub, ufloat_t))

    def __rmul__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return self.__mul__(other)
    
    def __rtruediv__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_op_rightsided(other, self, operator.truediv, ufloat_t))
    
    def __rfloordiv__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_op_rightsided(other, self, operator.floordiv, ufloat_t))
    
    def __rmod__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_op_rightsided(other, self, operator.fmod, ufloat_t))
    
    # unary ops

    def __abs__(self) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_unary_op(self.arr, abs))
    
    def __pos__(self) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_unary_op(self.arr, operator.pos))
    
    def __neg__(self) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_unary_op(self.arr, operator.neg))
    
    def __invert__(self) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_unary_op(self.arr, operator.invert))
    
    def __ceil__(self) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_unary_op(self.arr, umath.ceil))

    def __floor__(self) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_unary_op(self.arr, umath.floor))

    def __trunc__(self) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_unary_op(self.arr, umath.trunc))

    # bool ops

    def __eq__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_logic_op(self, other, operator.eq, ufloat_t))
    
    def __ne__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_logic_op(self, other, operator.ne, ufloat_t))
    
    def __lt__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_logic_op(self, other, operator.lt, ufloat_t))
    
    def __gt__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_logic_op(self, other, operator.gt, ufloat_t))
    
    def __le__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_logic_op(self, other, operator.le, ufloat_t))
    
    def __ge__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_logic_op(self, other, operator.ge, ufloat_t))
    
    def __req__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_logic_op_rightsided(other, self, operator.eq, ufloat_t))
    
    def __rne__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_logic_op_rightsided(other, self, operator.ne, ufloat_t))
    
    def __rlt__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_logic_op_rightsided(other, self, operator.lt, ufloat_t))
    
    def __rgt__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_logic_op_rightsided(other, self, operator.gt))
    
    def __rle__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_logic_op_rightsided(other, self, operator.le))
    
    def __rge__(self, other: Union[UncertaintiesArray, float, int, ufloat_t]) -> UncertaintiesArray:
        return UncertaintiesArray(go._generic_binary_logic_op_rightsided(other, self, operator.ge))

    # utils

    def __list__(self) -> List:
        return self.arr.tolist()

    def __repr__(self) -> str:
        return f'UncertaintiesArray({list(self)})'
    
    def __str__(self) -> str:
        return self.__repr__()
    
class UncertaintiesList(UncertaintiesArray):
    def __init__(self, a: Optional[Union[Iterable, UncertaintiesArray]]=None) -> None:
        super().__init__(a)

    def copy(self) -> UncertaintiesList:
        return UncertaintiesList(self)
    
    def values(self) -> SmartList:
        return SmartList((e.nominal_value for e in self), dtype=float)
    
    def errors(self) -> SmartList:
        return SmartList((e.std_dev for e in self), dtype=float)

    def append(self, x: ufloat_t) -> None:
        if not isinstance(x, ufloat_t):
            raise TypeError()
        self.arr.append(x)
    
    def extend(self, it: Iterable) -> None:
        if not all(isinstance(e, ufloat_t) for e in it):
            raise TypeError()
        self.arr.extend(it)

    def clear(self) -> None:
        self.arr.clear()

    def insert(self, i: int, x: ufloat_t) -> None:
        if not isinstance(x, ufloat_t):
            raise TypeError()
        self.arr.index(i, x)

    def pop(self, i: int) -> ufloat_t:
        return self.arr.pop(i)
    
    def __repr__(self) -> str:
        return f'UncertaintiesList({list(self)})'
