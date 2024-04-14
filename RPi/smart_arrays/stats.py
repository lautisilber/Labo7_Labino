from .smart_array_base import SmartArrayNumber, SmartListNumber
from .smart_array import SmartArrayInt
from typing import TypeVar, Union, Optional
import itertools

R = TypeVar('R', bound=float)

S = Union[SmartArrayNumber[R], SmartListNumber[R]]

def calculate_quartiles(arr: S) -> tuple[R, R, R]:
    a = arr.copy()
    a.sort()
    l = len(a)
    q1 = a[l//4]
    q2 = a[2 * l // 4]
    q3 = a[3 * l // 4]
    return q1, q2, q3

def interquartile_range_filter(arr: S) -> S:
    a = arr.copy()
    a.sort()
    l = len(a)

    q1 = a[l//4]
    q3 = a[3 * l // 4]

    # Interquartile range (IQR)
    iqr = q3 - q1

    # Define the lower and upper bounds
    lower = q1 - (1.5 * iqr)
    upper = q3 + (1.5 * iqr)
    
    return arr.copy(e for e in arr if lower <= e <= upper)

C = TypeVar('C', bound=complex)

def get_fastest_path(l: Union[SmartArrayNumber[C], SmartListNumber[C]], starting_position: Optional[C]=None) -> SmartArrayInt:
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
        return SmartArrayInt(range(len(l)))

    l = SmartListNumber[C](l)
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

    return SmartArrayInt(indices)
