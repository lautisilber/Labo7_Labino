import os, json, re
from typing import Any
from .smart_array_base import (
    SmartArray, SmartList, SmartArrayNumber, SmartListNumber
)
from .smart_array import (
    SmartArrayNumber, SmartArrayComplex, SmartArrayFloat, SmartArrayInt, SmartArrayBool,
    SmartListNumber, SmartListComplex, SmartListFloat, SmartListInt, SmartListBool
)

try:
    import uncertainties # type: ignore
    from .uncertainties_array import UncertaintiesArray, UncertaintiesList
    _uncertainties_exists = True
    classes = (SmartArray, SmartList, SmartArrayNumber, SmartListNumber,
            SmartArrayNumber, SmartArrayComplex, SmartArrayFloat, SmartArrayInt, SmartArrayBool,
            SmartListNumber, SmartListComplex, SmartListFloat, SmartListInt, SmartListBool,
            UncertaintiesArray, UncertaintiesList)
except ModuleNotFoundError:
    _uncertainties_exists = False
    classes = (SmartArray, SmartList, SmartArrayNumber, SmartListNumber,
            SmartArrayNumber, SmartArrayComplex, SmartArrayFloat, SmartArrayInt, SmartArrayBool,
            SmartListNumber, SmartListComplex, SmartListFloat, SmartListInt, SmartListBool)

def _camel_to_snake(name: str) -> str:
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()


name_class = {
    _camel_to_snake(c.__name__):c for c in classes
}
class_name = {
    c:_camel_to_snake(c.__name__) for c in classes
}

def savetxt(arr: SmartArray[Any], fname: str) -> None:
    dirname = os.path.dirname(fname)
    if not os.path.isdir(dirname):
        os.makedirs(dirname)
    if not hasattr(arr, '__class__'):
        raise TypeError('arr is not a class')
    if not any(arr.__class__ is c for c in classes):
        raise TypeError(f'{arr} is not one of the supported types')
    obj = {
        'class': class_name[arr.__class__],
        'arr': list(arr)
    }
    with open(fname, 'w') as f:
        json.dump(obj, f)

def loadtxt(fname: str) -> SmartArray[Any]:
    with open(fname, 'r') as f:
        obj = json.load(f)
    if not ('class' in obj and 'arr' in obj):
        raise ValueError('Bad file')
    if obj['class'] not in name_class.keys() or not isinstance(obj['arr'], list):
        raise ValueError('Bad file')
    return name_class[obj['class']](obj['arr'])

