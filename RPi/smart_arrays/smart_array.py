from .smart_array_base import SmartArrayNumber, SmartArrayReal, SmartListNumber, SmartListReal
from typing import Collection, Generator, Union
try:
    from typing import Self
except ImportError:
    from typing_extensions import Self


class SmartArrayFloat(SmartArrayReal[float]):
    def __init__(self, a: Union[Collection[float], Generator[float, None, None]], dtype: None=None) -> None:
        super().__init__(a, float)

    @classmethod
    def zeros(cls, n: int) -> Self:
        return cls.filled(n, 0.0)
    
class SmartArrayComplex(SmartArrayNumber[complex]):
    def __init__(self, a: Union[Collection[complex], Generator[complex, None, None]], dtype: None=None) -> None:
        super().__init__(a, complex)
    
    @classmethod
    def zeros(cls, n: int) -> Self:
        return cls.filled(n, complex(0, 0))
    
    def real(self) -> SmartArrayFloat:
        return SmartArrayFloat(c.real for c in self)
    
    def imag(self) -> SmartArrayFloat:
        return SmartArrayFloat(c.imag for c in self)

class SmartArrayInt(SmartArrayReal[int]):
    def __init__(self, a: Union[Collection[int], Generator[int, None, None]], dtype: None=None) -> None:
        super().__init__(a, int)

    @classmethod
    def zeros(cls, n: int) -> Self:
        return cls.filled(n, 0)

class SmartArrayBool(SmartArrayReal[bool]):
    def __init__(self, a: Union[Collection[bool], Generator[bool, None, None]], dtype: None=None) -> None:
        super().__init__(a, bool)

    @classmethod
    def zeros(cls, n: int) -> Self:
        return cls.filled(n, False)


class SmartListFloat(SmartArrayFloat, SmartListReal[float]):
    def __init__(self, a: Union[Collection[float], Generator[float, None, None]], dtype: None=None) -> None:
        super().__init__(a)

class SmartListComplex(SmartArrayComplex, SmartListNumber[complex]):
    def __init__(self, a: Union[Collection[complex], Generator[complex, None, None]], dtype: None=None) -> None:
        super(SmartArrayComplex, self).__init__(a)

    def real(self) -> SmartListFloat:
        return SmartListFloat(c.real for c in self)
    
    def imag(self) -> SmartListFloat:
        return SmartListFloat(c.imag for c in self)

class SmartListInt(SmartArrayInt, SmartListReal[int]):
    def __init__(self, a: Union[Collection[int], Generator[int, None, None]], dtype: None=None) -> None:
        super().__init__(a)

class SmartListBool(SmartArrayBool, SmartListReal[bool]):
    def __init__(self, a: Union[Collection[bool], Generator[bool, None, None]], dtype: None=None) -> None:
        super().__init__(a)