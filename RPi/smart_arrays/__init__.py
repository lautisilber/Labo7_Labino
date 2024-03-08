from ._check_dependencies import uncertainties_exists

from .smart_array import SmartArray, SmartList
if uncertainties_exists:
    from .uncertainties_array import UncertaintiesArray, UncertaintiesList
    __all__ = ['smart_array', 'uncertainties_array']
else:
    __all__ = ['smart_array']


