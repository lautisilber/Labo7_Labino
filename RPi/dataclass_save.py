import os
import json
import dataclasses
from typing import Optional, get_origin, Any, TypeVar, Type

T = TypeVar('T')


def _populate_dataclass_from_dict(cls: Type[T], example: T, obj: dict) -> T:
    field_types = {f.name: f.type for f in dataclasses.fields(cls)}
    new_obj = dict()

    for field_name, field_type in field_types.items():
        if not field_name in obj: continue
        # attr needs to be in example class as well
        if not hasattr(example, field_name):
            raise TypeError()
        example_attr = getattr(example, field_name)
        # check if dataclass
        if dataclasses.is_dataclass(example_attr): # or dataclasses.is_dataclass(field_type):
            new_obj[field_name] = _populate_dataclass_from_dict(example_attr.__class__, example_attr, obj[field_name])
        # check if list
        elif isinstance(obj[field_name], (tuple, list)):
            # type checks against example
            if not isinstance(example_attr, (tuple, list)):
                raise TypeError()
            if not len(example_attr) == len(obj[field_name]):
                raise IndexError()
            # populate new list
            l = list()
            for e, example_e in zip(obj[field_name], example_attr):
                if dataclasses.is_dataclass(example_e):
                    l.append(_populate_dataclass_from_dict(example_e.__class__, example_e, e))
                else:
                    l.append(type(example_e)(e))
            # tuple or list
            if field_type is tuple or get_origin(field_type) is tuple:
                l = tuple(l)
            # assign new list
            new_obj[field_name] = l
        else:
            new_obj[field_name] = type(example_attr)(obj[field_name])
    return cls(**new_obj)

def load_dataclass(defaults: T, save_file: Optional[str]=None) -> T:
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
        if hasattr(defaults, 'save_file'):
            save_file = defaults.save_file
        else:
            raise ValueError(f'defaults and save_file arguments are both None. Cannot load {cls}')
    file_exists = os.path.isfile(save_file)
    if not file_exists:
        return dataclasses.replace(defaults)
    else:
        with open(save_file, 'r') as f:
            d = json.load(f)
        return _populate_dataclass_from_dict(cls, defaults, d)

def save_dataclass(cls_instance: T, save_file: Optional[str]=None) -> bool:
    if not dataclasses.is_dataclass(cls_instance.__class__):
        raise TypeError('cls is not a dataclass')
    if save_file is None:
        if hasattr(cls_instance, 'save_file'):
            save_file = cls_instance.save_file
        else:
            Exception('Instance has no attribute \'save_file\'')
    dirname = os.path.dirname(save_file)
    if dirname and not os.path.isdir(dirname):
        os.makedirs(dirname)
    obj = dataclasses.asdict(cls_instance)
    if not isinstance(obj, dict):
        raise TypeError('__dict__ method should return a dict')
    with open(save_file, 'w') as f:
        json.dump(obj, f)


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

    r = load_dataclass(C, c, fname)
    print(r)
    save_dataclass(r, fname)
