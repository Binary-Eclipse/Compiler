# def, return, recursion, defaults, keyword args, scope, global
def square(number):
    return number * number

def greet(name, greeting="Hello"):
    return greeting + ", " + name + "!"

def add(a, b):
    return a + b

def fact(n):
    if n <= 1:
        return 1
    return n * fact(n - 1)

def fib(n):
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)

def no_return():
    x = 5

def early(n):
    for i in range(10):
        if i == n:
            return i * 100
    return -1

counter = 0
def bump():
    global counter
    counter += 1
    return counter

def local_only():
    counter = 99
    return counter

def stats(values):
    total = 0
    for v in values:
        total += v
    return total / len(values)

print(square(7), add(10, 20), add("a", "b"), add([1], [2]))
print(greet("Shakil"), greet("Rahim", "Hi"), greet(greeting="Yo", name="Kim"))
print(fact(10), fact(20), fib(15))
print(no_return(), early(3), early(50))
print(bump(), bump(), bump(), counter)
print(local_only(), counter)
print(stats([80, 65, 90, 95]))
f = square
print(f(9))
def apply_twice(g, x):
    return g(g(x))
print(apply_twice(square, 3))
def outer():
    def inner(v):
        return v + 1
    return inner(10)
print(outer())
def collatz(n):
    steps = 0
    while n != 1:
        if n % 2 == 0:
            n = n // 2
        else:
            n = 3 * n + 1
        steps += 1
    return steps
print(collatz(27))
def depth(n):
    if n == 0:
        return 0
    return 1 + depth(n - 1)
print(depth(900))
def modify(lst):
    lst.append(99)
    lst = [0]
    return lst
data = [1, 2]
print(modify(data), data)
