from __future__ import annotations
from array import array
from typing import Iterable, Optional, Literal, Union, List
from array import array
from typing import Union
import operator
import math

typecodes = ('b', 'B', 'u', 'h', 'H', 'i', 'I', 'l', 'L', 'q', 'Q', 'f', 'd')
typecode_t = Union[
    Literal['b'],
    Literal['B'],
    Literal['u'],
    Literal['h'],
    Literal['H'],
    Literal['i'],
    Literal['I'],
    Literal['l'],
    Literal['L'],
    Literal['q'],
    Literal['Q'],
    Literal['f'],
    Literal['d']
]


def __generic_binary_op(a: array, b: Union[array, int, float], op) -> array:
    t = float if a.typecode in ('f', 'd') else int
    if isinstance(b, (float, int)):
        return array(a.typecode, (t(op(e,b)) for e in a))
    elif isinstance(b, array):
        if not len(a) == len(b):
            raise IndexError('Arrays are not of same size')
        return array(a.typecode, (t(op(e1, e2)) for e1, e2 in zip(a, b)))
    raise TypeError()

def __generic_binary_op_rightsided(b: Union[array, int, float], a: array, op) -> array:
    t = float if a.typecode in ('f', 'd') else int
    if isinstance(b, (float, int)):
        return array(a.typecode, (t(op(b,e)) for e in a))
    elif isinstance(b, array):
        if not len(a) == len(b):
            raise IndexError('Arrays are not of same size')
        return array(a.typecode, (t(op(e1, e2)) for e1, e2 in zip(b, a)))
    raise TypeError()

# def _generic_binary_non_commutative_op(a: Union[array, int, float], b: Union[array, int, float], op) -> array:
#     if isinstance(a, array):
#         if isinstance(b, (float, int)):
#             return array(a.typecode, (op(e,b) for e in a))
#         elif isinstance(b, array):
#             if not len(a) == len(b):
#                 raise IndexError('Arrays are not of same size')
#             return array(a.typecode, (op(e1, e2) for e1, e2 in zip(a, b)))
#     elif isinstance(a, (float, int)):
#         if isinstance(b, (float, int)):
#             raise TypeError('Neither a nor b are arrays')
#         elif isinstance(b, array):
#             return array(b.typecode, (op(a, e) for e in b))
#     raise TypeError()

def __generic_unary_op(a: array, op, t: Optional[typecode_t]=None) -> array:
    t = a.typecode if t is None else t
    return array(t, (op(e) for e in a))

def __generic_binary_logic_op(a: array, b: Union[array, int, float], op) -> array:
    if isinstance(b, (float, int)):
        return array('B', (op(e,b) for e in a))
    elif isinstance(b, array):
        if not len(a) == len(b):
            raise IndexError('Arrays are not of same size')
        return array('B', (op(e1, e2) for e1, e2 in zip(a, b)))
    raise TypeError()

def __generic_binary_logic_op_rightsided(b: Union[array, int, float], a: array, op) -> array:
    if isinstance(b, (float, int)):
        return array('B', (op(b,e) for e in a))
    elif isinstance(b, array):
        if not len(a) == len(b):
            raise IndexError('Arrays are not of same size')
        return array('B', (op(e1, e2) for e1, e2 in zip(b, a)))
    raise TypeError()

def filled(size: int, value: Union[int, float], t: Optional[typecode_t]=None) -> SmartArray:
    if size < 0:
        raise ValueError()
    return SmartArray([value]*size, t)

def zeros(size: int, t: Optional[typecode_t]=None) -> SmartArray:
    return filled(size, 0, t)

def sqrt(a: SmartArray) -> SmartArray:
    return __generic_unary_op(a, math.sqrt)

class SmartArray:
    def __init__(self, a: Optional[Iterable]=None, t: Optional[typecode_t]=None) -> None:
        if t is None:
            if isinstance(a, (array, SmartArray)):
                t = a.typecode
            else:
                t = 'd'

        if not t in typecodes:
            raise ValueError()
        if a is None:
            self.arr = array(t)
        else:
            self.arr = array(t, a)

    @property
    def typecode(self) -> typecode_t:
        return self.arr.typecode

    @property
    def size(self) -> int:
        return len(self.arr)
    
    def __len__(self) -> int:
        return self.size
    
    def __iter__(self):
        return iter(self.arr)
    
    def __getitem__(self, item: int) -> Union[int, float]:
        return self.arr[item]
    
    def bool(self, t: Optional[typecode_t]=None) -> SmartArray:
        t = 'B' if t is None else t
        return SmartArray(__generic_unary_op(self.arr, bool, t))
    
    def int(self, t: Optional[typecode_t]=None) -> SmartArray:
        if t is None:
            t = 'q' if self.typecode not in ('b', 'B', 'u', 'h', 'H', 'i', 'I', 'l', 'L', 'q', 'Q') else self.typecode
        return SmartArray(__generic_unary_op(self.arr, int, t))
    
    def float(self, t: Optional[typecode_t]=None) -> SmartArray:
        if t is None:
            t = 'd' if self.typecode not in ('f', 'd') else self.typecode
        return SmartArray(__generic_unary_op(self.arr, float, t))
    
    def append(self, x: Union[int, float]) -> None:
        self.arr.append(x)

    def count(self, x: Union[int, float]) -> int:
        return self.arr.count(x)
    
    def extend(self, it: Iterable) -> None:
        self.arr.extend(it)
    
    # math ops

    def __add__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.add))
    
    def __sub__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.sub))
    
    def __mul__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.mul))
    
    def __truediv__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.truediv))
    
    def __floordiv__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.floordiv))
    
    def __pow__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.pow))
    
    def __mod__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.mod))
    
    def __radd__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return self.__add__(other)
    
    def __rsub__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.sub))

    def __rmul__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return self.__mul__(other)
    
    def __rtruediv__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.truediv))
    
    def __rfloordiv__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.floordiv))
    
    def __rmod__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.mod))
    
    # unary ops

    def __abs__(self) -> SmartArray:
        return SmartArray(__generic_unary_op(self.arr, abs))
    
    def __pos__(self) -> SmartArray:
        return SmartArray(__generic_unary_op(self.arr, operator.pos))
    
    def __neg__(self) -> SmartArray:
        return SmartArray(__generic_unary_op(self.arr, operator.neg))
    
    def __invert__(self) -> SmartArray:
        return SmartArray(__generic_unary_op(self.arr, operator.invert))
    
    def __ceil__(self) -> SmartArray:
        return SmartArray(__generic_unary_op(self.arr, math.ceil))

    def __floor__(self) -> SmartArray:
        return SmartArray(__generic_unary_op(self.arr, math.floor))

    def __trunc__(self) -> SmartArray:
        return SmartArray(__generic_unary_op(self.arr, math.trunc))

    # logic ops

    def __and__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.and_))
    
    def __or__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.or_))
    
    def __xor__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.xor))

    def __lshift__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.lshift))
    
    def __rshift__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.rshift))

    def __rand__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.and_))

    def __ror__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.or_))
    
    def __rxor__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.xor))

    def __rlshift__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.lshift))
    
    def __rrshift__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.rshift))

    # bool ops

    def __eq__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.eq))
    
    def __ne__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.ne))
    
    def __lt__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.lt))
    
    def __gt__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.gt))
    
    def __le__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.le))
    
    def __ge__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op(self.arr, other if isinstance(other, (float, int)) else other.arr, operator.ge))
    
    def __req__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.eq))
    
    def __rne__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.ne))
    
    def __rlt__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.lt))
    
    def __rgt__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.gt))
    
    def __rle__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.le))
    
    def __rge__(self, other: Union[SmartArray, float, int]) -> SmartArray:
        return SmartArray(__generic_binary_logic_op_rightsided(other if isinstance(other, (float, int)) else other.arr, self.arr, operator.ge))

    # utils

    def __list__(self) -> List:
        return self.arr.tolist()
    
    def tolist(self) -> List:
        return self.__list__()

    def __repr__(self) -> str:
        return f'SmartArray({self.arr.tolist()})'
    
    def __str__(self) -> str:
        return self.__repr__()
