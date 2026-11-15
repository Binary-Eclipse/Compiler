# and / or / not, comparisons, identity, membership, precedence
a = 5
b = 10
print(a < b, a > b, a <= 5, b >= 11, a == 5, a != 5)
print(a < b and b < 20, a > b or b > 5, not a > b, not (a < b))
print(1 and 2, 0 and 2, 1 or 2, 0 or 2, "" or "x", "a" and "b", None or [], 0 or None)
print(not 0, not 1, not "", not "x", not [], not None, not not 5)
print(True and False or True, True or False and False, not True or True, not (True or True))
x = None
print(x is None, x is not None, 5 is not None, None is None, [] is not None)
print(3 in [1, 2, 3], "a" in ["b"], [1] in [[1], [2]], 4 not in [1, 2, 3])
print(1 + 2 < 4 and 2 * 3 == 6, 1 + 1 == 2 or 1 / 1 == 0)
print(-3 ** 2, (-3) ** 2, 2 ** 3 ** 2, 10 - 2 - 3, 100 / 10 / 5, 2 * 3 % 4)
print(not 1 == 2, not 1 in [1], 1 < 2 and not 3 < 2)
age = 20
has_id = True
if age >= 18 and has_id:
    print("allowed")
if not (age < 18) or not has_id:
    print("allowed 2")
def check(n):
    if n % 3 == 0 and n % 5 == 0:
        return "FizzBuzz"
    elif n % 3 == 0:
        return "Fizz"
    elif n % 5 == 0:
        return "Buzz"
    return n
for i in range(1, 16):
    print(check(i))
print(True == True, True != False, 1 == True, 0 == False, "" == False)
print(2 < 3.5, 2.5 > 2, "b" > "a", [1, 2] != [1, 3], None != 0)
