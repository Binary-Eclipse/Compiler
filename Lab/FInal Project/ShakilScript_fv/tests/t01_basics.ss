# Output, variables, arithmetic, data types
print("Hello World!")
print("Welcome to ShakilScript!")
name = "Shakil"
age = 24
cgpa = 3.85
is_student = True
nothing = None
print(name, age, cgpa, is_student, nothing)
age = 25
age += 1
print("age is", age)
print(10 + 5, 10 - 5, 10 * 5, 10 / 4, 10 // 3, 10 % 3, 2 ** 8)
print(10 / 2, 7 / 2, -7 // 2, -7 % 3, 7 % -3, 7.5 // 2, 7.5 % 2)
print(2 ** -1, 2 ** 0.5, 10 ** 15, (-2) ** 3, -2 ** 2)
print(1 + 2 * 3 - 4 / 2, (1 + 2) * (3 - 4) / 2)
print(5 // 2 * 2 + 5 % 2)
print("a", "b", "c", sep="-")
print("no newline", end="")
print(" <- same line")
print("x", 1, 2.0, [1, "a", None, True, 2.5])
print()
print("tab\there", 'single "quotes"', "it's")
count = 0
count += 1
count *= 5
count -= 2
count //= 2
count **= 3
count %= 5
print(count)
print(1.0, 2.50, 1e3, 1.5e-7, 123456789.123456789, 1e16, 1e15, 0.0001, 0.00001)
print(True + True, True * 5, -True)
print(abs(-3), abs(-2.5), abs(4), min(3, 1, 2), max([4, 9, 2]), sum([1, 2, 3, 4]), sum([0.1] * 10))
