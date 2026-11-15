# 05 - Functions: def, return, calling, recursion
task add_numbers(a, b):
    give a + b

task greet(name, greeting="Hello"):
    give greeting + ", " + name + "!"

task factorial(n):
    when n <= 1:
        give 1
    give n * factorial(n - 1)

show(add_numbers(10, 20))
show(greet("Shakil"))
show(greet("Rahim", "Welcome"))
show("5! =", factorial(5))
show("10! =", factorial(10))

counter = 0
task bump():
    global counter        # change the outer variable
    counter += 1
bump()
bump()
show("counter:", counter)
