from __future__ import annotations
from abc import ABC
from typing import TypeVar, Optional, Iterable, Generic, Any, Union, Callable, Generator, Iterator, Self

T = TypeVar('T')
U = TypeVar('U')
V = TypeVar('V')

def castable(v: Any, dtype: type) -> bool:
    try:
        dtype(v)
        return True
    except ValueError:
        return False

class SmartArrayBase(ABC, Generic[T]):
    def __init__(self, a: Iterable[T], dtype: Optional[type[T]]=None) -> None:
        self.arr: list[T] = list(a)
        if dtype:
            if not all(castable(e, dtype) for e in self.arr):
                raise TypeError(f'Not all elements of a were of type {dtype}')
            self.t: type[T] = dtype
            self.arr = [self.t(e) for e in self.arr]
        else:
            if len(self.arr) == 0:
                raise Exception('Cannot instantiate a SmartArrayBase with an empty a and a dtype == None')
            self.t = type(self.arr[0])
        if not all(isinstance(e, self.t) for e in self.arr):
            raise TypeError(f'Not all elements of a are of tye {self.dtype}')
        
    @property
    def dtype(self) -> type[T]:
        return self.t
            
    def __iter__(self) -> Iterator[T]:
        return self.arr.__iter__
    
    def __getitem__(self, key: int) -> T:
        return self.arr[key]
    
    def __setitem__(self, key: int, value: T):
        self.arr[key] = self.t(value)

    def __len__(self) -> int:
        return len(self.arr)

    def __bool__(self) -> bool:
        return self.__len__() > 0
    
    @classmethod
    def filled(cls, n: int, v: T, dtype: Optional[type[T]]=None) -> Self[T]:
        return cls([v]*n, dtype=dtype)
    
    @staticmethod
    def _binary_op(this: Union[Iterable[T], T], that: Union[Iterable[U], U], op: Callable[[T, U], Union[T, U, V]]) -> Generator[Union[T, U, V], None, None]:
        this_iter = isinstance(this, Iterable)
        that_iter = isinstance(this, Iterable)
        if this_iter:
            if that_iter:
                if not len(this) == len(that):
                    raise ValueError('length of iterables does not match')
                return (op(a, b) for a, b in zip(this, that))
            return (op(a, that) for a in this)
        if that_iter:
            return (op(this, b) for b in that)
        raise TypeError('Neither is an iterable')
    
    @staticmethod
    def _binary_logic_op(this: Union[Iterable[T], T], that: Union[Iterable[U], U], op: Callable[[T, U], bool]) -> Generator[bool, None, None]:
        return SmartArrayBase._binary_logic_op(this, that, op)
    
    def _unary_op(self, op: Callable[[T], Union[T, U]]) -> Generator[Union[T, U], None, None]:
        return (op(e) for e in self)
    
    def _binary_op_left(self, other: Union[Iterable[U], U], op: Callable[[T, U], Union[T, U, V]]) -> Generator[Union[T, U, V], None, None]:
        return SmartArrayBase._binary_op(self, other, op)
    
    def _binary_op_right(self, other: Union[Iterable[U], U], op: Callable[[T, U], Union[T, U, V]]) -> Generator[Union[T, U, V], None, None]:
        return SmartArrayBase._binary_op(other, self, op)
    
    def _binary_logic_op_left(self, other: Union[Iterable[U], U], op: Callable[[T, U], bool]) -> Generator[bool, None, None]:
        return self._binary_logic_op_left(other, op)
    
    def _binary_logic_op_right(self, other: Union[Iterable[U], U], op: Callable[[T, U], bool]) -> Generator[bool, None, None]:
        return self._binary_op_right(other, op)
    
    def reverse(self) -> None:
        self.arr.reverse()

    def sort(self, *args, key: Optional[Callable]=None, reverse: bool=False) -> None:
        self.arr.sort(*args, key=key, reverse=reverse)

    def copy(self) -> Self[T]:
        return self.__class__(self)
    
    def __repr__(self) -> str:
        return f'{self.__class__.__name__}({list(self)})'
    
    def __str__(self) -> str:
        return self.__repr__()

    
class SmartListBase(SmartArrayBase[T], ABC):
    def __init__(self, a: Iterable[T], dtype: Optional[type[T]] = None) -> None:
        super().__init__(a, dtype)
    
    def append(self, x: T) -> None:
        if not castable(x, self.dtype):
            raise TypeError(f'{x} cannot be casted to type {self.dtype}')
        self.arr.append(self.dtype(x))
    
    def extend(self, it: Iterable[T]) -> None:
        if not all(castable(e, self.dtype) for e in it):
            raise TypeError(f'Not all elements of it can be casted to type {self.dtype}')
        self.arr.extend([self.dtype(e) for e in it])

    def clear(self) -> None:
        self.arr.clear()

    def insert(self, i: int, x: T) -> None:
        if not castable(x, self.dtype):
            raise TypeError(f'{x} cannot be casted to type {self.dtype}')
        self.arr.index(i, x)

    def pop(self, i: int) -> T:
        return self.arr.pop(i)
    

### typed base

import operator as op
from numbers import Real
from statistics import mean, stdev

class SmartArrayRealBase(SmartArrayBase[T]):
    def __init__(self, a: Iterable[T], dtype: Optional[type[T]] = None) -> None:
        super().__init__(a, dtype)
        if not issubclass(self.dtype, Real):
            raise TypeError(f'dtype is {self.dtype} instead of {Real}')
    
    def bool(self) -> Self[bool]:
        return self.__class__[bool](self, dtype=bool)
    
    def int(self) -> Self[int]:
        return self.__class__[int](self, dtype=int)
    
    def float(self) -> Self[float]:
        return self.__class__[float](self, dtype=float)
    
    def mean(self) -> Real:
        return mean(self)
    
    def stdev(self) -> Real:
        return stdev(self)
    
    # math ops

    def __add__(self, other: Union[Iterable[T], T]) -> Self[T]:
        return self.__class__[T](self._binary_logic_op_left(other, op.add))
    
    def __sub__(self, other: Union[Iterable[T], T]) -> Self[T]:
        return self.__class__[T](self._binary_logic_op_left(other, op.sub))
    
    def __mul__(self, other: Union[Iterable[T], T]) -> Self[T]:
        return self.__class__[T](self._binary_logic_op_left(other, op.mul))
    
    def __truediv__(self, other: Union[Iterable[T], T]) -> Self[float]:
        return self.__class__[float](self._binary_logic_op_left(other, op.truediv), dtype=float)
    
    def __floordiv__(self, other: Union[Iterable[T], T]) -> Self[int]:
        return self.__class__[int](self._binary_logic_op_left(other, op.floordiv), dtype=int)
    
    def __pow__(self, other: Union[Iterable[T], T]) -> Self[T]:
        return self.__class__[T](self._binary_logic_op_left(other, op.pow))
    
    def __mod__(self, other: Union[Iterable[T], T]) -> Self[T]:
        return self.__class__[T](self._binary_logic_op_left(other, op.mod))
    
    def __radd__(self, other: Union[Iterable[T], T]) -> Self[T]:
        return self.__add__(other)
    
    def __rsub__(self, other: Union[Iterable[T], T]) -> Self[T]:
        return self.__class__[T](self._binary_logic_op_right(other, op.sub))

    def __rmul__(self, other: Union[Iterable[T], T]) -> Self[T]:
        return self.__mul__(other)
    
    def __rtruediv__(self, other: Union[Iterable[T], T]) -> Self[T]:
        return self.__class__[T](self._binary_logic_op_right(other, op.truediv), dtype=float)
    
    def __rfloordiv__(self, other: Union[Iterable[T], T]) -> Self[T]:
        return self.__class__[T](self._binary_logic_op_right(other, op.floordiv), dtype=int)
    
    def __rmod__(self, other: Union[Iterable[T], T]) -> Self[T]:
        return self.__class__[T](self._binary_logic_op_right(other, op.mod))
    
    # unary ops

    def __abs__(self) -> Self[T]:
        return self.__class__[T](self._unary_op(op.abs))
    
    def __pos__(self) -> Self[T]:
        return self.__class__[T](self._unary_op(op.pos))
    
    def __neg__(self) -> Self[T]:
        return self.__class__[T](self._unary_op(op.neg))
    
    def __invert__(self) -> Self[T]:
        return self.__class__[T](self._unary_op(op.invert))

    # bool ops

    def __eq__(self, other: Union[Iterable[T], T]) -> Self[bool]:
        return self.__class__[bool](self._binary_logic_op_left(other, op.eq), dtype=bool)
    
    def __ne__(self, other: Union[Iterable[T], T]) -> Self[bool]:
        return self.__class__[bool](self._binary_logic_op_left(other, op.ne), dtype=bool)
    
    def __lt__(self, other: Union[Iterable[T], T]) -> Self[bool]:
        return self.__class__[bool](self._binary_logic_op_left(other, op.lt), dtype=bool)
    
    def __gt__(self, other: Union[Iterable[T], T]) -> Self[bool]:
        return self.__class__[bool](self._binary_logic_op_left(other, op.gt), dtype=bool)
    
    def __le__(self, other: Union[Iterable[T], T]) -> Self[bool]:
        return self.__class__[bool](self._binary_logic_op_left(other, op.le), dtype=bool)
    
    def __ge__(self, other: Union[Iterable[T], T]) -> Self[bool]:
        return self.__class__[bool](self._binary_logic_op_left(other, op.ge), dtype=bool)
    
    def __req__(self, other: Union[Iterable[T], T]) -> Self[bool]:
        return self.__class__[bool](self._binary_logic_op_right(other, op.eq), dtype=bool)
    
    def __rne__(self, other: Union[Iterable[T], T]) -> Self[bool]:
        return self.__class__[bool](self._binary_logic_op_right(other, op.ne), dtype=bool)
    
    def __rlt__(self, other: Union[Iterable[T], T]) -> Self[bool]:
        return self.__class__[bool](self._binary_logic_op_right(other, op.lt), dtype=bool)
    
    def __rgt__(self, other: Union[Iterable[T], T]) -> Self[bool]:
        return self.__class__[bool](self._binary_logic_op_right(other, op.gt), dtype=bool)
    
    def __rle__(self, other: Union[Iterable[T], T]) -> Self[bool]:
        return self.__class__[bool](self._binary_logic_op_right(other, op.le), dtype=bool)
    
    def __rge__(self, other: Union[Iterable[T], T]) -> Self[bool]:
        return self.__class__[bool](self._binary_logic_op_right(other, op.ge), dtype=bool)
    
class SmartListRealBase(SmartArrayRealBase[T], SmartListBase[T]):
    def __init__(self, a: Iterable[T], dtype: Optional[type[T]] = None) -> None:
        super(SmartArrayRealBase, self).__init__(a, dtype)