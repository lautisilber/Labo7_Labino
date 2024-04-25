import os
import json
import dataclasses
from typing import Optional, get_origin, Any, TypeVar, Type
from typing import ClassVar, Dict, Protocol, Any


class IsDataclass(Protocol):
    # as already noted in comments, checking for this attribute is currently
    # the most reliable way to ascertain that something is a dataclass
    __dataclass_fields__: ClassVar[Dict[str, Any]] 

T = TypeVar('T', bound=IsDataclass)

def _populate_dataclass_from_dict(cls: Type[T], example: T, obj: dict[str, Any]) -> T:
    field_types = {f.name: f.type for f in dataclasses.fields(cls)}
    new_obj: dict[str, Any] = dict()

    for field_name, field_type in field_types.items():
        if not field_name in obj: continue
        # attr needs to be in example class as well
        if not hasattr(example, field_name):
            raise TypeError()
        example_attr: Any = getattr(example, field_name)
        # check if dataclass
        if dataclasses.is_dataclass(example_attr): # or dataclasses.is_dataclass(field_type):
            new_obj[field_name] = _populate_dataclass_from_dict(example_attr.__class__, example_attr, obj[field_name])
        # check if list
        elif isinstance(obj[field_name], (tuple, list)):
            # type checks against example
            if not isinstance(example_attr, (tuple, list)):
                raise TypeError()
            if not len(example_attr) == len(obj[field_name]): # type: ignore
                raise IndexError()
            # populate new list
            l: list[Any] = list()
            for e, example_e in zip(obj[field_name], example_attr): # type: ignore
                if dataclasses.is_dataclass(example_e): # type: ignore
                    l.append(_populate_dataclass_from_dict(example_e.__class__, example_e, e)) # type: ignore
                else:
                    l.append(type(example_e)(e)) # type: ignore
            # tuple or list
            if field_type is tuple or get_origin(field_type) is tuple:
                l = tuple(l) # type: ignore
            # assign new list
            new_obj[field_name] = l
        else:
            new_obj[field_name] = type(example_attr)(obj[field_name])
    return cls(**new_obj)

def load_dataclass_json(defaults: T, save_file: Optional[str]=None) -> T:
    '''
        Loads a dataclass of type type(cls) from the file save_file
        If defaults is not None, it should be of same type as cls
        and represents the defaul values cls should take if data cannot
        be loaded from save_file
    '''
    if not hasattr(defaults, '__class__'):
        raise TypeError('defaults is not a class')
    cls: Type[T] = defaults.__class__
    if not dataclasses.is_dataclass(cls):
        raise TypeError('cls is not a dataclass')
    if save_file is None:
        if hasattr(defaults, '_save_file'):
            save_file = defaults._save_file # type: ignore
        else:
            raise ValueError(f'defaults._save_file and save_file arguments are both None. Cannot load {cls}')
    file_exists = os.path.isfile(save_file) # type: ignore
    if not file_exists:
        return dataclasses.replace(defaults)
    else:
        with open(save_file, 'r') as f: # type: ignore
            d = json.load(f) # type: ignore
        return _populate_dataclass_from_dict(cls, defaults, d)

def save_dataclass_json(cls_instance: IsDataclass, save_file: Optional[str]=None) -> None:
    if not dataclasses.is_dataclass(cls_instance.__class__):
        raise TypeError('cls is not a dataclass')
    if save_file is None:
        if hasattr(cls_instance, '_save_file'):
            save_file = cls_instance._save_file # type: ignore
        else:
            raise Exception('Instance has no attribute \'save_file\'')
    dirname = os.path.dirname(save_file) # type: ignore
    if dirname and not os.path.isdir(dirname): # type: ignore
        os.makedirs(dirname) # type: ignore
    obj = dataclasses.asdict(cls_instance)
    if not isinstance(obj, dict): # type: ignore
        raise TypeError('__dict__ method should return a dict')
    with open(save_file, 'w') as f: # type: ignore
        json.dump(obj, f) # type: ignore





if __name__ == '__main__':
    @dataclasses.dataclass
    class A:
        a: int
        b: str
        c: tuple[int,...]
    
    @dataclasses.dataclass
    class B:
        a: tuple[A,...]
        b: tuple[int,...]
        c: int

    @dataclasses.dataclass
    class C:
        a: tuple[A,...]
        b: tuple[B,...]
        c: tuple[int,...]
        d: int
    
    a = A(1, 'chau', (2, 3, 4))
    b = B((a, a), (5, 6, 7), 8)
    c = C(
        a=(a, a, a),
        b=(b,b,b,b),
        c=(9,10,11,12,13),
        d=14
    )

    fname = '.dataclass_save_test.json'

    r = load_dataclass_json(C, c, fname) # type: ignore
    print(r) # type: ignore
    save_dataclass_json(r, fname) # type: ignore
