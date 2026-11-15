# if / elif / else, while, for, break, continue
score = 72
if score >= 90:
    print("A")
elif score >= 80:
    print("B")
elif score >= 70:
    print("C")
else:
    print("F")

age = 20
if age >= 18:
    print("You are an adult")
else:
    print("You are a minor")

if age > 100: print("old")
else: print("young")

for i in range(5):
    print("i =", i)
for i in range(2, 10, 3):
    print(i, end=" ")
print()
for i in range(5, 0, -2):
    print(i, end=" ")
print()
for ch in "abc":
    print(ch)
for item in [10, "x", 3.5]:
    print(item)

n = 0
while n < 10:
    n += 1
    if n == 3:
        continue
    if n == 7:
        break
    print("n", n)
else_ran = "loop done"
print(else_ran, n)

total = 0
for i in range(1, 101):
    total += i
print("sum 1..100 =", total)

for i in range(1, 4):
    for j in range(1, 4):
        if j == 2:
            continue
        if i == 3:
            break
        print(i, j)

i = 0
while True:
    i += 1
    if i > 3:
        break
print(i)
for k in range(3):
    pass
print(k)
found = False
for v in [4, 8, 15, 16, 23, 42]:
    if v % 5 == 0:
        found = v
        break
print(found)
