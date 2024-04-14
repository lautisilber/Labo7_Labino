import os
import itertools
from typing import Any, Optional, Union, Generator, TypeVar
from collections import deque
import pickle

def get_files_directory(f: str) -> str:
    '''
    should be used like this
        get_files_directory(__file__)
    '''
    abspath = os.path.abspath(__file__)
    dname = os.path.dirname(abspath)
    return dname

# def get_fastest_path(l: Union[list[int], tuple[int,...]], starting_position: Optional[int]) -> tuple[int,...]:
#     # returns the indices that arange the original list in the shortest path (given that it must start on the first position)
#     # https://en.m.wikipedia.org/wiki/Travelling_salesman_problem
#     if len(l) < 2:
#         return tuple(l)
#     if starting_position is None:
#         l_init = l[0]
#         l_rest = l[1:]
#         tot_abs_sum = lambda _l: sum(abs(_l[i]-_l[i+1]) for i in range(len(_l)-1))
#     else:
#         if starting_position in l:
#             tot_abs_sum = lambda _l: sum(abs(_l[i]-_l[i+1]) for i in range(len(_l)-1))
#             l_init = starting_position
#             l_rest = l.copy()
#             l_rest.remove(starting_position)
#         else:

#     permutations = itertools.permutations(l_rest)

#     best_path = [l_init] + list(next(permutations))
#     min_total_abs_diff = tot_abs_sum(best_path)

#     # find best permutation
#     for perm in permutations:
#         contesting_path = [l_init] + list(perm)
#         contesting_path_cost = tot_abs_sum(contesting_path)
#         if contesting_path_cost < min_total_abs_diff:
#             best_path = contesting_path
#             min_total_abs_diff = contesting_path_cost

#     # get indices of permutation
#     indices = list()
#     for e in best_path:
#         for i, o in enumerate(l):
#             if o == e and i not in indices:
#                 indices.append(i)
#                 break

#     return tuple(indices)

T = TypeVar('T')

def slice_deque(d: deque[T], stop: int, start: int=0, step: int=1, length_check: bool=True) -> Generator[T, None, None]:
    if length_check:
        stop = min(stop, len(d))
        start = min(start, stop)
    return (d[i] for i in range(start, stop, step))


def save_pickle(obj: Any, save_file: Optional[str]=None) -> None:
    if save_file is None:
        if hasattr(obj, '_save_file'):
            save_file = obj._save_file
            if not isinstance(save_file, str):
                raise ValueError(f'save_file str gotten from obj._save_file is not str. it is {type(save_file)}')
    d = os.path.dirname(save_file) # type: ignore
    if d:
        if not os.path.isdir(d):
            os.makedirs(d)
    with open(save_file, 'wb') as f: # type: ignore
        pickle.dump(obj, f)

def load_pickle(default: T, save_file: Optional[str]=None) -> T:
    if save_file is None:
        if hasattr(default, '_save_file'):
            save_file = default._save_file # type: ignore
            if not isinstance(save_file, str):
                raise ValueError(f'save_file str gotten from default._save_file is not str. it is {type(save_file)}')
        else:
            raise ValueError(f'default._save_file and save_file arguments are both None. Cannot load {type(default)}')
    file_exists = os.path.isfile(save_file)
    if not file_exists:
        return default
    with open(save_file, 'rb') as f:
        obj = pickle.load(f)
    return obj
