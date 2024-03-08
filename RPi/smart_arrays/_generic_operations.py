from typing import Iterable, Optional, Union, Tuple, Callable
from ._check_dependencies import uncertainties_exists

if uncertainties_exists:
    from uncertainties.core import AffineScalarFunc as ufloat_t
    scalar_t = Union[bool, int, float, complex, ufloat_t]
else:
    scalar_t = Union[bool, int, float, complex]

def _generic_binary_op(a: Iterable, b: Union[Iterable, scalar_t], op: Callable, force_type: Optional[type]=None) -> Tuple: # type: ignore
    if isinstance(b, scalar_t):
        return tuple(op(e,b) for e in a)
    elif isinstance(b, Iterable):
        if not len(a) == len(b):
            raise IndexError('Arrays are not of same size')
        if force_type is not None:
            if not (all(isinstance(e, force_type) for e in a) and all(isinstance(e, force_type) for e in a)):
                raise TypeError()
        return tuple(op(e1, e2) for e1, e2 in zip(a, b))
    raise TypeError()

def _generic_binary_op_rightsided(b: Union[Iterable, scalar_t], a: Iterable, op: Callable, force_type: Optional[type]=None) -> Tuple: # type: ignore
    if isinstance(b, scalar_t):
        return tuple(op(b,e) for e in a)
    elif isinstance(b, Iterable):
        if not len(a) == len(b):
            raise IndexError('Arrays are not of same size')
        if force_type is not None:
            if not (all(isinstance(e, force_type) for e in a) and all(isinstance(e, force_type) for e in a)):
                raise TypeError()
        return tuple(op(e1, e2) for e1, e2 in zip(b, a))
    raise TypeError()

def _generic_unary_op(a: Iterable, op) -> Tuple: # type: ignore
    return tuple(op(e) for e in a)

def _generic_binary_logic_op(a: Iterable, b: Union[Iterable, scalar_t], op: Callable, force_type: Optional[type]=None) -> Tuple: # type: ignore
    if isinstance(b, scalar_t):
        return tuple(op(e,b) for e in a)
    elif isinstance(b, Iterable):
        if not len(a) == len(b):
            raise IndexError('Arrays are not of same size')
        if force_type is not None:
            if not (all(isinstance(e, force_type) for e in a) and all(isinstance(e, force_type) for e in a)):
                raise TypeError()
        return tuple(op(e1, e2) for e1, e2 in zip(a, b))
    raise TypeError()

def _generic_binary_logic_op_rightsided(b: Union[Iterable, scalar_t], a: Iterable, op: Callable, force_type: Optional[type]=None) -> Tuple: # type: ignore
    if isinstance(b, scalar_t):
        return tuple(op(b,e) for e in a)
    elif isinstance(b, Iterable):
        if not len(a) == len(b):
            raise IndexError('Arrays are not of same size')
        if force_type is not None:
            if not (all(isinstance(e, force_type) for e in a) and all(isinstance(e, force_type) for e in a)):
                raise TypeError()
        return tuple(op(e1, e2) for e1, e2 in zip(b, a))
    raise TypeError()