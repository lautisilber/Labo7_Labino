from typing import Any, Iterable

def castable(v: Any, t: type) -> bool:
    c = False
    try:
        t(v)
        c = True
    except ValueError:
        c = False
    return c

def calculate_dominant_type(t1: type, t2: type) -> type:
    ts = (complex, float, int, bool)
    for t in ts:
        if t1 is t or t2 is t:
            return t
    raise TypeError()

def calculate_dominant_type_from_iter(it: Iterable) -> type:
    ts = (complex, float, int, bool)
    # ls = list(ts) # TODO: remove
    for t in ts:
        if any(isinstance(e, t) for e in it):
            return t
    raise TypeError()