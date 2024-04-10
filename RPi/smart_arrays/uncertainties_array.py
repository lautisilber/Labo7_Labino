from typing import Iterable
from .smart_array_base import SmartArrayBase, SmartListBase
from .smart_array import SmartArrayFloat, SmartListFloat
from uncertainties import ufloat, umath
from typing import Optional, Union, Self
from numbers import Real
import operator as op

ufloat_or_real = Union[ufloat, Real]

class UncertaintiesArray(SmartArrayBase[ufloat]):
    def __init__(self, a: Union[Iterable[Real], Iterable[ufloat]], b: Optional[Iterable[Real]]=None) -> None:
        if b is not None:
            b = list(b)
            if not len(a) == len(b):
                raise ValueError('a and b are not of same length')
            if not (all(isinstance(e, Real) for e in a) and all(isinstance(e, Real) for e in b)):
                raise TypeError(f'Not all values of a or b are of type {Real}')
            a: list[ufloat] = [ufloat(e1, e2) for e1, e2 in zip(a, b)]
        else:
            if not all(isinstance(e, ufloat) for e in a):
                raise TypeError(f'Not all values of a are of type {ufloat}')
            a: list[ufloat] = list(a)
        super().__init__(a, ufloat)

    def values(self) -> SmartArrayFloat:
        return SmartArrayFloat([e.nominal_value for e in self.arr])
    
    def errors(self) -> SmartArrayFloat:
        return SmartArrayFloat([e.std_dev for e in self.arr])
    
    @classmethod
    def zeros(cls, n: int) -> Self:
        return cls.filled(n, ufloat(0.0, 0.0), ufloat)
    
    # math ops

    def __add__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_left(other, op.add))
    
    def __sub__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_left(other, op.sub))
    
    def __mul__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_left(other, op.mul))
    
    def __truediv__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_left(other, op.truediv), dtype=float)
    
    def __floordiv__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_left(other, op.floordiv), dtype=int)
    
    def __pow__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_left(other, umath.pow))
    
    def __radd__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__add__(other)
    
    def __rsub__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_right(other, op.sub))

    def __rmul__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__mul__(other)
    
    def __rtruediv__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_right(other, op.truediv), dtype=float)
    
    def __rfloordiv__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_right(other, op.floordiv), dtype=int)
    
    # unary ops

    def __abs__(self) -> Self:
        return self.__class__(self._unary_op(op.abs))
    
    def __pos__(self) -> Self:
        return self.__class__(self._unary_op(op.pos))
    
    def __neg__(self) -> Self:
        return self.__class__(self._unary_op(op.neg))
    
    def __invert__(self) -> Self:
        return self.__class__(self._unary_op(op.invert))

    # bool ops

    def __eq__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_left(other, op.eq), dtype=bool)
    
    def __ne__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_left(other, op.ne), dtype=bool)
    
    def __lt__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_left(other, op.lt), dtype=bool)
    
    def __gt__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_left(other, op.gt), dtype=bool)
    
    def __le__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_left(other, op.le), dtype=bool)
    
    def __ge__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_left(other, op.ge), dtype=bool)
    
    def __req__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_right(other, op.eq), dtype=bool)
    
    def __rne__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_right(other, op.ne), dtype=bool)
    
    def __rlt__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_right(other, op.lt), dtype=bool)
    
    def __rgt__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_right(other, op.gt), dtype=bool)
    
    def __rle__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_right(other, op.le), dtype=bool)
    
    def __rge__(self, other: Union[Iterable[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(self._binary_logic_op_right(other, op.ge), dtype=bool)
    

class UncertaintiesList(UncertaintiesArray, SmartListBase):
    def __init__(self, a: Union[Iterable[Real], Iterable[ufloat]], b: Optional[Iterable[Real]] = None) -> None:
        super(UncertaintiesArray, self).__init__(a, b)

    def values(self) -> SmartListFloat:
        return SmartListFloat([e.nominal_value for e in self.arr])
    
    def errors(self) -> SmartListFloat:
        return SmartListFloat([e.std_dev for e in self.arr])