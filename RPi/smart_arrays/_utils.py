from typing import Any, Iterable, Union

scalar_t = Union[bool, int, float, complex]

def castable(v: Any, t: scalar_t) -> bool:
    c = False
    try:
        t(v)
        c = True
    except ValueError:
        c = False
    return c

def calculate_dominant_type(t1: type, t2: type) -> scalar_t:
    ts = (complex, float, int, bool)
    for t in ts:
        if t1 is t or t2 is t:
            return t
    raise TypeError()

def calculate_dominant_type_from_iter(it: Iterable) -> scalar_t:
    ts = (bool, int, float, complex)
    # ls = list(ts) # TODO: remove
    for t in ts:
        if all(isinstance(e, t) for e in it):
            return t
    raise TypeError()
