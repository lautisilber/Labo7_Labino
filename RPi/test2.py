from smart_arrays import SmartArrayInt


a = SmartArrayInt(range(20))

print(a)

a[::2] = [-4 for _ in range(10)]

print(a)