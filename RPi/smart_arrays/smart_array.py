from .smart_array_base import SmartArrayRealBase, SmartListRealBase
from typing import TypeVar, Optional, Iterable, Generic, Any, Union, Self
from numbers import Real
import operator as op


class SmartArrayReal(SmartArrayRealBase[Real]):
    def __init__(self, a: Iterable[Real], dtype: Optional[Real] = None) -> None:
        super().__init__(a, dtype)

class SmartArrayFloat(SmartArrayRealBase[float]):
    def __init__(self, a: Iterable[float]) -> None:
        super().__init__(a)

    @classmethod
    def zeros(cls, n: int) -> Self:
        return cls.filled(n, 0.0, float)

class SmartArrayInt(SmartArrayRealBase[int]):
    def __init__(self, a: Iterable[int]) -> None:
        super().__init__(a)

    @classmethod
    def zeros(cls, n: int) -> Self:
        return cls.filled(n, 0, int)

class SmartArrayBool(SmartArrayRealBase[bool]):
    def __init__(self, a: Iterable[bool]) -> None:
        super().__init__(a)

    @classmethod
    def zeros(cls, n: int) -> Self:
        return cls.filled(n, False, bool)


class SmartListReal(SmartListRealBase[Real]):
    def __init__(self, a: Iterable[Real], dtype: Optional[Real] = None) -> None:
        super().__init__(a, dtype)

class SmartListFloat(SmartListRealBase[float]):
    def __init__(self, a: Iterable[float]) -> None:
        super().__init__(a)

class SmartListInt(SmartListRealBase[int]):
    def __init__(self, a: Iterable[int]) -> None:
        super().__init__(a)

    @classmethod
    def zeros(cls, n: int) -> Self:
        return cls.filled(n, 0.0, float)

    @classmethod
    def zeros(cls, n: int) -> Self:
        return cls.filled(n, 0, int)

class SmartListBool(SmartListRealBase[bool]):
    def __init__(self, a: Iterable[bool]) -> None:
        super().__init__(a)

    @classmethod
    def zeros(cls, n: int) -> Self:
        return cls.filled(n, False, bool)