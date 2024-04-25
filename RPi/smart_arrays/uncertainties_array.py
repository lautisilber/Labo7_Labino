from typing import Collection
from .smart_array_base import SmartArray, SmartList
from .smart_array import SmartArrayFloat, SmartListFloat
from uncertainties import ufloat, umath # type: ignore
from uncertainties.core import AffineScalarFunc as ufloat_type # type: ignore
from typing import Optional, Union, Self, Collection
# from numbers import Real
import operator as op

ufloat_or_real = Union[ufloat_type, float]


class UncertaintiesArray(SmartArray[ufloat_type]):
    def __init__(self, a: Union[Collection[ufloat_type], Collection[float]], b: Optional[Collection[float]]=None) -> None:
        if b is not None:
            b = list(b)
            if not len(a) == len(b):
                raise ValueError('a and b are not of same length')
            if not (all(isinstance(e, float) for e in a) and all(isinstance(e, float) for e in b)):
                raise TypeError(f'Not all values of a or b are of type {float}')
            a_: list[ufloat_type] = [ufloat(e1, e2) for e1, e2 in zip(a, b)]
        else:
            if not all(isinstance(e, ufloat_type) for e in a):
                raise TypeError(f'Not all values of a are of type ufloat ({ufloat_type})')
            a_: list[ufloat_type] = list(a) # type: ignore
        super().__init__(a_, ufloat_type, _skip_type_checking=True)

    def values(self) -> SmartArrayFloat:
        return SmartArrayFloat([e.nominal_value for e in self.arr])
    
    def errors(self) -> SmartArrayFloat:
        return SmartArrayFloat([e.std_dev for e in self.arr])
    
    @classmethod
    def zeros(cls, n: int) -> Self:
        return cls.filled(n, ufloat(0.0, 0.0)) # type: ignore
    
    # math ops

    def __add__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_op_left(other, op.add)))
    
    def __sub__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self: # type: ignore
        return self.__class__(tuple(self._binary_op_left(other, op.sub)))
    
    def __mul__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_op_left(other, op.mul)))
    
    def __truediv__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_op_left(other, op.truediv)))
    
    def __floordiv__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_op_left(other, op.floordiv)))
    
    def __pow__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_op_left(other, umath.pow))) # type: ignore
    
    def __radd__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__add__(other)
    
    def __rsub__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_op_right(other, op.sub)))

    def __rmul__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__mul__(other)
    
    def __rtruediv__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_op_right(other, op.truediv)))
    
    def __rfloordiv__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_op_right(other, op.floordiv)))
    
    def __rpow__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_op_right(other, umath.pow))) # type: ignore
    
    # unary ops

    def __abs__(self) -> Self:
        return self.__class__(tuple(self._unary_op(op.abs))) # type: ignore
    
    def __pos__(self) -> Self:
        return self.__class__(tuple(self._unary_op(op.pos))) # type: ignore
    
    def __neg__(self) -> Self:
        return self.__class__(tuple(self._unary_op(op.neg))) # type: ignore

    # bool ops

    def __eq__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self: # type: ignore
        return self.__class__(tuple(self._binary_logic_op_left(other, op.eq))) # type: ignore
    
    def __ne__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self: # type: ignore
        return self.__class__(tuple(self._binary_logic_op_left(other, op.ne))) # type: ignore
    
    def __lt__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_logic_op_left(other, op.lt))) # type: ignore
    
    def __gt__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_logic_op_left(other, op.gt))) # type: ignore
    
    def __le__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_logic_op_left(other, op.le))) # type: ignore
    
    def __ge__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_logic_op_left(other, op.ge))) # type: ignore
    
    def __req__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__eq__(other)
    
    def __rne__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__ne__(other)
    
    def __rlt__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_logic_op_right(other, op.lt))) # type: ignore
    
    def __rgt__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_logic_op_right(other, op.gt))) # type: ignore
    
    def __rle__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_logic_op_right(other, op.le))) # type: ignore
    
    def __rge__(self, other: Union[Collection[ufloat_or_real], ufloat_or_real]) -> Self:
        return self.__class__(tuple(self._binary_logic_op_right(other, op.ge))) # type: ignore
    

class UncertaintiesList(UncertaintiesArray, SmartList[ufloat_type]):
    def __init__(self, a: Union[Collection[ufloat_type], Collection[float]], b: Optional[Collection[float]]=None) -> None:
        super(UncertaintiesArray, self).__init__(a, b) # type: ignore

    def values(self) -> SmartListFloat:
        return SmartListFloat([e.nominal_value for e in self.arr])
    
    def errors(self) -> SmartListFloat:
        return SmartListFloat([e.std_dev for e in self.arr])