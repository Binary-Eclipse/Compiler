# All 25 ShakilScript functions in Python format
#  1 print     2 input     3 create    4 update    5 get
#  6 add       7 subtract  8 multiply  9 divide   10 modulo
# 11 compare  12 when     13 repeat   14 stop     15 skip
# 16 return   17 call     18 sort     19 find     20 at
# 21 append   22 remove   23 count    24 reverse  25 convert
name = input("Enter your name: ")            # 2 input, 3 create
text = input("Enter a number: ")
number = int(text)                           # 25 convert
print("Hello,", name)                        # 1 print
number = number + 1                          # 4 update, 6 add
print(number)                                # 5 get
print(number - 3, number * 2, number / 4, number % 3)   # 7, 8, 9, 10
print(number > 5, number == 10, number != 3)             # 11 compare
if number > 5:                               # 12 when
    print("big")
else:
    print("small")
total = 0
for i in range(10):                          # 13 repeat
    if i == 2:
        continue                             # 15 skip
    if i == 6:
        break                                # 14 stop
    total += i
print("total", total)
def square(n):
    return n * n                             # 16 return
print(square(7))                             # 17 call
scores = [80, 65, 90, 72]
scores.append(95)                            # 21 append
scores.sort()                                # 18 sort
print(scores)
print(scores.index(90), scores[0], scores[-1])   # 19 find, 20 at
scores.remove(65)                            # 22 remove
scores.reverse()                             # 24 reverse
print(scores, len(scores))                   # 23 count
print(int("123") + 1, float("2.5"), str(12) + "!", bool(0))   # 25 convert
