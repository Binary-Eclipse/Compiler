# 05 - Functions: def, return, calling, recursion
def add_numbers(a, b):
    return a + b

def greet(name, greeting="Hello"):
    return greeting + ", " + name + "!"

def factorial(n):
    if n <= 1:
        return 1
    return n * factorial(n - 1)

print(add_numbers(10, 20))
print(greet("Shakil"))
print(greet("Rahim", "Welcome"))
print("5! =", factorial(5))
print("10! =", factorial(10))

counter = 0
def bump():
    global counter        # change the outer variable
    counter += 1
bump()
bump()
print("counter:", counter)
