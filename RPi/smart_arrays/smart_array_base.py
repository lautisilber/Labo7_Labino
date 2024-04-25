from __future__ import annotations
import sys
from typing import Iterable, TypeVar, Optional, Collection, Sequence, Any, Union, Callable, Generator, Iterator, Set, MutableSequence, MutableSet, overload
try:
    from typing import Self
except ImportError:
    from typing_extensions import Self


T = TypeVar('T')
U = TypeVar('U')
V = TypeVar('V')
W = TypeVar('W')

def castable(v: Any, dtype: type) -> bool:
    try:
        dtype(v)
        return True
    except ValueError:
        return False
    
from math import ceil
def get_range_length(start: int, stop: int, step: int=0) -> int:
    return ceil((stop - start) / step)

class SmartArray(Sequence[T]):
    def __init__(self, a: Union[Collection[T], Generator[T, None, None]], dtype: Optional[type[T]]=None, _skip_type_checking: bool=False) -> None:
        self.arr: list[T] = list(a)
        self._should_cast: bool = False
        if dtype:
            if not _skip_type_checking:
                if not all(isinstance(e, dtype) for e in self.arr):
                    if all(castable(e, dtype) for e in self.arr):
                        self._should_cast = True
                    else:
                        raise TypeError(f'Not all elements of a were of type {dtype}')
                self.t: type[T] = dtype
                self.arr = [self.t(e) if self._should_cast else e for e in self.arr]
        else:
            if len(self.arr) == 0:
                raise Exception('Cannot instantiate a SmartArrayBase with an empty a and a dtype == None')
            self.t = type(self.arr[0])
        if not _skip_type_checking:
            if not all(isinstance(e, self.t) for e in self.arr):
                raise TypeError(f'Not all elements of a are of tye {self.dtype}')
        
    @property
    def dtype(self) -> type[T]:
        return self.t
            
    def __iter__(self) -> Iterator[T]:
        return self.arr.__iter__()
    
    @overload
    def __getitem__(self, key: slice) -> Self:
        ...
    
    @overload
    def __getitem__(self, key: int) -> T:
        ...
    
    def __getitem__(self, key: Union[int, slice]) -> Union[T, Self]: # type: ignore
        if isinstance(key, slice):
            return self.__class__(self.arr[key])
        else:
            return self.arr[key]
    
    @overload
    def __setitem__(self, key: slice, value: Union[T, Sequence[T]]) -> None:
        ...

    @overload
    def __setitem__(self, key: int, value: T) -> None:
        ...

    def __setitem__(self, key: Union[int, slice], value: Union[T, Sequence[T]]) -> None:
        if isinstance(key, slice):
            s_len = len(self)
            start: Optional[int] = key.start
            stop: Optional[int] = key.stop
            step: Optional[int] = key.step
            if start is None: start = 0
            if stop is None: stop = s_len
            if step is None: step = 1

            slice_length = get_range_length(start, stop, step)
            if stop > s_len:
                raise IndexError(f'Slice stops at {stop}, when length is {s_len}')
            if isinstance(value, Sequence):
                if len(value) != slice_length: # type: ignore
                    raise IndexError(f'Index mismatch. The slice contained {slice_length} indices. value has length {len(value)}') # type: ignore
                if not self._should_cast:
                    if not all(isinstance(e, self.t) for e in value): # type: ignore
                        raise TypeError(f'Not all elements of value are of type {self.t}')
                for i, j in enumerate(range(start, stop, step)):
                    if self._should_cast:
                        self.arr[j] = self.t(value[i])
                    else:
                        self.arr[j] = value[i]
            else:
                if not self._should_cast:
                    if not isinstance(value, self.t):
                        raise TypeError(f'value is not of type {self.t}')
                for i in range(start, stop, step):
                    if self._should_cast:
                        self.arr[i] = self.t(value)
                    else:
                        self.arr[i] = value
        else:
            if self._should_cast:
                self.arr[key] = self.t(value)
            else:
                if not isinstance(value, self.t):
                    raise TypeError(f'value is not of type {self.t}')
                else:
                    self.arr[key] = value

    def __contains__(self, item: Any) -> bool:
        return item in self.arr
    
    def __reversed__(self) -> Iterator[T]:
        return self.arr.__reversed__()

    def __len__(self) -> int:
        return len(self.arr)

    def __bool__(self) -> bool:
        return self.__len__() > 0
    
    def index(self, value: T, start: int=0, stop: int=sys.maxsize) -> int:
        return self.arr.index(value, start, stop)
    
    def count(self, value: T) -> int:
        return self.arr.count(value)
    
    @classmethod
    def filled(cls, n: int, v: T, dtype: Optional[type[T]]=None) -> Self:
        if dtype is None:
            return cls([v]*n)
        else:
            return cls([v]*n, dtype=dtype)
    
    @staticmethod
    def _binary_op(this: Union[Collection[U], U], that: Union[Collection[V], V], op: Callable[[U, V], W]) -> Generator[W, None, None]:
        if isinstance(this, Collection):
            if isinstance(that, Collection):
                if not len(this) == len(that): # type: ignore
                    raise ValueError('length of iterables does not match')
                return (op(a, b) for a, b in zip(this, that)) # type: ignore
            else:
                return (op(a, that) for a in this) # type: ignore
        elif isinstance(that, Collection):
            return (op(this, b) for b in that) # type: ignore
        raise TypeError('Neither is an iterable')
    
    @staticmethod
    def _binary_logic_op(this: Union[Collection[U], U], that: Union[Collection[V], V], op: Callable[[U, V], bool]) -> Generator[bool, None, None]:
        return SmartArray._binary_op(this, that, op)
    
    def _unary_op(self, op: Callable[[T], U]) -> Generator[U, None, None]:
        return (op(e) for e in self)
    
    def _binary_op_left(self, other: Union[Collection[U], U], op: Callable[[T, U], V]) -> Generator[V, None, None]:
        return SmartArray._binary_op(self, other, op) # type: ignore
    
    def _binary_op_right(self, other: Union[Collection[U], U], op: Callable[[U, T], V]) -> Generator[V, None, None]:
        return SmartArray._binary_op(other, self, op) # type: ignore
    
    def _binary_logic_op_left(self, other: Union[Collection[U], U], op: Callable[[T, U], bool]) -> Generator[bool, None, None]:
        return SmartArray._binary_logic_op(self, other, op) # type: ignore
    
    def _binary_logic_op_right(self, other: Union[Collection[U], U], op: Callable[[U, T], bool]) -> Generator[bool, None, None]:
        return SmartArray._binary_logic_op(other, self, op) # type: ignore
    
    def _in_place_binary_op(self, other: Union[Collection[U], U], op: Callable[[T, U], T]) -> None:
        for i, v in enumerate(SmartArray._binary_op(self, other, op)): # type: ignore
            self[i] = v

    def _in_place_unary_op(self, op: Callable[[T], T]) -> None:
        for i, v in enumerate(SmartArray._unary_op(self, op)): # type: ignore
            self[i] = v
    
    def reverse(self) -> None:
        self.arr.reverse()

    def sort(self, *args: tuple[Any,...], key: Optional[Callable[[T], Any]]=None, reverse: bool=False) -> None:
        self.arr.sort(*args, key=key, reverse=reverse) # type: ignore

    def copy(self, a: Optional[Union[Collection[T], Generator[T, None, None]]]=None) -> Self:
        if a is None:
            return self.__class__(self)
        else:
            return self.__class__(a)
    
    def __repr__(self) -> str:
        return f'{self.__class__.__name__}({list(self)})'
    
    def __str__(self) -> str:
        return self.__repr__()

    
class SmartList(SmartArray[T], MutableSequence[T]):
    def __init__(self, a: Union[Collection[T], Generator[T, None, None]], dtype: Optional[type[T]] = None) -> None:
        super().__init__(a, dtype)
    
    def append(self, value: T) -> None:
        if not castable(value, self.dtype):
            raise TypeError(f'{value} cannot be casted to type {self.dtype}')
        self.arr.append(self.dtype(value))
    
    def extend(self, values: Iterable[T]) -> None:
        if not all(castable(e, self.dtype) for e in values):
            raise TypeError(f'Not all elements of it can be casted to type {self.dtype}')
        self.arr.extend([self.dtype(e) for e in values])

    def clear(self) -> None:
        self.arr.clear()

    def insert(self, index: int, value: T) -> None:
        if not castable(value, self.dtype):
            raise TypeError(f'{value} cannot be casted to type {self.dtype}')
        self.arr.insert(index, value)

    def pop(self, index: int=-1) -> T:
        return self.arr.pop(index)
    
    def remove(self, value: T) -> None:
        self.arr.remove(value)
    

### typed base

import operator as op
from numbers import Complex, Real
from statistics import mean, stdev
import math

C = TypeVar('C', bound=complex) # its meant to signify complex, floats, ints and bools

class SmartArrayNumber(SmartArray[C], Set[C]): # type: ignore
    def __init__(self, a: Union[Collection[C], Generator[C, None, None]], dtype: Optional[type[C]] = None) -> None:
        super().__init__(a, dtype)
        if not issubclass(self.dtype, Complex):
            raise TypeError(f'dtype is {self.dtype} instead of {Complex}')
    
    def bool(self) -> Self:
        return self.__class__(self, dtype=bool)
    
    def int(self) -> Self:
        return self.__class__(self, dtype=int)
    
    def float(self) -> Self:
        return self.__class__(self, dtype=float)
    
    def mean(self) -> C:
        return mean(self) # type: ignore
    
    def stdev(self) -> C:
        return stdev(self) # type: ignore
    
    # math ops

    def __add__(self, other: Union[Set[C], C]) -> Self:
        return self.__class__(self._binary_op_left(other, op.add))
    
    def __sub__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        return self.__class__(self._binary_op_left(other, op.sub))
    
    def __mul__(self, other: Union[Set[C], C]) -> Self:
        return self.__class__(self._binary_op_left(other, op.mul))
    
    def __truediv__(self, other: Union[Set[C], C]) -> Self:
        return self.__class__(self._binary_op_left(other, op.truediv), dtype=float)
    
    def __floordiv__(self, other: Union[Set[C], C]) -> Self:
        return self.__class__(self._binary_op_left(other, op.floordiv), dtype=int)
    
    def __pow__(self, other: Union[Set[C], C]) -> Self:
        return self.__class__(self._binary_op_left(other, op.pow))
    
    def __mod__(self, other: Union[Set[C], C]) -> Self:
        return self.__class__(self._binary_op_left(other, op.mod))
    
    def __radd__(self, other: Union[Set[C], C]) -> Self:
        return self.__add__(other)
    
    def __rsub__(self, other: Union[Set[C], C]) -> Self:
        return self.__class__(self._binary_op_right(other, op.sub))

    def __rmul__(self, other: Union[Set[C], C]) -> Self:
        return self.__mul__(other)
    
    def __rtruediv__(self, other: Union[Set[C], C]) -> Self:
        return self.__class__(self._binary_op_right(other, op.truediv), dtype=float)
    
    def __rfloordiv__(self, other: Union[Set[C], C]) -> Self:
        return self.__class__(self._binary_op_right(other, op.floordiv), dtype=int)
    
    def __rmod__(self, other: Union[Set[C], C]) -> Self:
        return self.__class__(self._binary_op_right(other, op.mod))
    
    def __rpow__(self, other: Union[Set[C], C]) -> Self:
        return self.__class__(self._binary_op_right(other, op.pow))
    
    # in-place math ops

    def __iadd__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        self._in_place_binary_op(other, op.add)
        return self

    def __isub__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        self._in_place_binary_op(other, op.sub)
        return self
    
    def __imul__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        self._in_place_binary_op(other, op.mul)
        return self
    
    def __itruediv__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        self._in_place_binary_op(other, op.truediv)
        return self
    
    def __ifloordiv__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        self._in_place_binary_op(other, op.floordiv)
        return self
    
    def __imod__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        self._in_place_binary_op(other, op.mod)
        return self
    
    def __ipow__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        self._in_place_binary_op(other, op.pow)
        return self
    
    # unary ops

    def __abs__(self) -> Self:
        return self.__class__(self._unary_op(op.abs)) # type: ignore
    
    def __pos__(self) -> Self:
        return self.__class__(self._unary_op(op.pos)) # type: ignore
    
    def __neg__(self) -> Self:
        return self.__class__(self._unary_op(op.neg)) # type: ignore
    
    def __invert__(self) -> Self:
        return self.__class__(self._unary_op(op.invert)) # type: ignore

    # bool ops

    def __eq__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        return self.__class__(self._binary_logic_op_left(other, op.eq), dtype=bool)
    
    def __ne__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        return self.__class__(self._binary_logic_op_left(other, op.ne), dtype=bool)
    
    def __lt__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        return self.__class__(self._binary_logic_op_left(other, op.lt), dtype=bool) # type: ignore
    
    def __gt__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        return self.__class__(self._binary_logic_op_left(other, op.gt), dtype=bool) # type: ignore
    
    def __le__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        return self.__class__(self._binary_logic_op_left(other, op.le), dtype=bool) # type: ignore
    
    def __ge__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        return self.__class__(self._binary_logic_op_left(other, op.ge), dtype=bool) # type: ignore
    
    def __req__(self, other: Union[Set[C], C]) -> Self:
        return self.__eq__(other)
    
    def __rne__(self, other: Union[Set[C], C]) -> Self:
        return self.__ne__(other)
    
    def __rlt__(self, other: Union[Set[C], C]) -> Self:
        return self.__gt__(other)
    
    def __rgt__(self, other: Union[Set[C], C]) -> Self:
        return self.__lt__(other)
    
    def __rle__(self, other: Union[Set[C], C]) -> Self:
        return self.__ge__(other)
    
    def __rge__(self, other: Union[Set[C], C]) -> Self:
        return self.__le__(other)
    
    # binary ops

    def __and__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        return self.__class__(self._binary_logic_op_left(other, op.and_))
    
    def __or__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        return self.__class__(self._binary_logic_op_left(other, op.or_))
    
    def __xor__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        return self.__class__(self._binary_logic_op_left(other, op.xor))
    
    # in-place binary ops

    def __iand__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        self._in_place_binary_op(other, op.and_)
        return self
    
    def __ior__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        self._in_place_binary_op(other, op.or_)
        return self
    
    def __ixor__(self, other: Union[Set[C], C]) -> Self: # type: ignore
        self._in_place_binary_op(other, op.xor)
    
    def isdisjoint(self, other: Iterable[Any]) -> bool:
        for e in other:
            if e in self:
                return False
        return True
    
class SmartListNumber(SmartArrayNumber[C], SmartList[C], MutableSet[C]): # type: ignore
    def __init__(self, a: Union[Collection[C], Generator[C, None, None]], dtype: Optional[type[C]]=None) -> None:
        super(SmartArrayNumber, self).__init__(a, dtype)

R = TypeVar('R', bound=float)

class SmartArrayReal(SmartArrayNumber[R]):
    def __init__(self, a: Union[Collection[R], Generator[R, None, None]], dtype: Optional[type[R]]=None) -> None:
        super().__init__(a, dtype)
        if not issubclass(self.dtype, Real):
            raise TypeError(f'dtype is {self.dtype} instead of {Real}')

    def sqrt(self) -> Self:
        self._in_place_unary_op(math.sqrt) # type: ignore
        return self
    
class SmartListReal(SmartArrayReal[R], SmartListNumber[R]):
    def __init__(self, a: Union[Collection[R], Generator[R, None, None]], dtype: Optional[type[R]]=None) -> None:
        super(SmartArrayReal, self).__init__(a, dtype)
