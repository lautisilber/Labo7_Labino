import os
import itertools
from typing import Any, Optional, Union, List, Tuple, Generator, TypeVar
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

def get_fastest_path(l: Union[List[Union[int, float]], Tuple[Union[int, float],...]], starting_position: Optional[Union[int, float]]=None) -> Tuple[int,...]:
    '''
    Takes a list of positions (1D) and finds the arrangement of positions (given that the starting position is fixed)
    so that the sum of the absolute differences between contiguous positions is minimized.
    return: tuple with the indices of the original list that correspond to the resulting configuration
    l: the list to analyze
    starting_position: Is the fixed starting position the resulting configuration must have. If it is in l, all good and the
                       resulting configuration will start with the indec corresponding to this position. If it is not in l,
                       the sum of absolute differences will be calculated as if there was a starting element in the position
                       starting_position, but the returned tuple of indices will correspond to the given list l. If starting_position
                       is None, it is equal to the first element of l.
    '''
    # https://en.m.wikipedia.org/wiki/Travelling_salesman_problem
    if len(l) < 2:
        return tuple(l)

    if starting_position is None:
        starting_position = l[0]
    if starting_position in l:
        l_init = starting_position
        l_rest = l.copy()
        l_rest.remove(starting_position)
    else:
        l_init = starting_position
        l_rest = l.copy()

    tot_abs_sum = lambda _l: sum(abs(_l[i]-_l[i+1]) for i in range(len(_l)-1))

    permutations = itertools.permutations(l_rest)

    best_path = [l_init] + list(next(permutations))
    min_total_abs_diff = tot_abs_sum(best_path)

    # find best permutation
    for perm in permutations:
        contesting_path = [l_init] + list(perm)
        contesting_path_cost = tot_abs_sum(contesting_path)
        if contesting_path_cost < min_total_abs_diff:
            best_path = contesting_path
            min_total_abs_diff = contesting_path_cost

    # get indices of permutation
    indices = list()
    for e in best_path[1 if starting_position not in l else 0:]:
        for i, o in enumerate(l):
            if o == e and i not in indices:
                indices.append(i)
                break

    return tuple(indices)

# def get_fastest_path(l: Union[List[Union[int, float]], Tuple[Union[int, float],...]], starting_position: Optional[Union[int, float]]) -> Tuple[int,...]:
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


def slice_deque(d: deque, stop: int, start: int=0, step: int=1, length_check: bool=True) -> Generator:
    if length_check:
        stop = min(stop, len(d))
        start = min(start, stop)
    return (d[i] for i in range(start, stop, step))


T = TypeVar('T')

def save_pickle(obj: Any, save_file: Optional[str]=None) -> None:
    if save_file is None:
        if hasattr(obj, '_save_file'):
            save_file = obj._save_file
    d = os.path.dirname(save_file)
    if d:
        if not os.path.isdir(d):
            os.makedirs(d)
    with open(save_file, 'wb') as f:
        pickle.dump(obj, f)

def load_pickle(default: T, save_file: Optional[str]=None) -> T:
    if save_file is None:
        if hasattr(default, '_save_file'):
            save_file = default._save_file
        else:
            raise ValueError(f'default._save_file and save_file arguments are both None. Cannot load {type(default)}')
    file_exists = os.path.isfile(save_file)
    if not file_exists:
        return default
    with open(save_file, 'rb') as f:
        obj = pickle.load(f)
    return obj