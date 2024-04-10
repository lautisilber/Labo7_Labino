from __future__ import annotations
import numpy as np
import numpy.typing as npt
from typing import Annotated, Literal, Type
from uncertainties import unumpy as unp

np_float = np.float64
np_arr_float = npt.NDArray[np_float]
np_arr_float_1D = Annotated[np_arr_float, Literal[1]] # second argument is ndim
np_arr_float_2D = Annotated[np_arr_float, Literal[2]] # second argument is ndim
np_arr_float_3D = Annotated[np_arr_float, Literal[3]] # second argument is ndim

np_int = np.int32
np_arr_int = npt.NDArray[np_int]
np_arr_int_1D = Annotated[np_arr_int, Literal[1]] # second argument is ndim
np_arr_int_2D = Annotated[np_arr_int, Literal[2]] # second argument is ndim
np_arr_int_3D = Annotated[np_arr_int, Literal[3]] # second argument is ndim

np_bool = np.bool_
np_arr_bool = npt.NDArray[np_bool]
np_arr_bool_1D = Annotated[np_arr_bool, Literal[1]] # second argument is ndim
np_arr_bool_2D = Annotated[np_arr_bool, Literal[2]] # second argument is ndim
np_arr_bool_3D = Annotated[np_arr_bool, Literal[3]] # second argument is ndim