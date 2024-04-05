from typing import TypeVar, Generic, get_args

T = TypeVar('T')

class Graph(Generic[T], object):
    def get_generic_type(self):
        return self.__orig_class__.__args__[0]
    def copy(self):
        return Graph[self.get_generic_type()]()
    
def foo(v: T):
    print(get_args(T))


if __name__=='__main__':
    g_int = Graph[int]()
    g_str = Graph[str]()
    h = g_int.copy()

    print(g_int.get_generic_type())
    print(g_str.get_generic_type())
    print(h.get_generic_type())

    foo(1)

   