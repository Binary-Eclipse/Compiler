# All 25 ShakilScript functions in Python format
#  1 print     2 input     3 create    4 update    5 get
#  6 add       7 subtract  8 multiply  9 divide   10 modulo
# 11 compare  12 when     13 repeat   14 stop     15 skip
# 16 return   17 call     18 sort     19 find     20 at
# 21 append   22 remove   23 count    24 reverse  25 convert
name = ask("Enter your name: ")            # 2 input, 3 create
raw = ask("Enter a number: ")
number = whole(raw)                          # 25 convert
show("Hello,", name)                        # 1 print
number = number + 1                          # 4 update, 6 add
show(number)                                # 5 get
show(number - 3, number * 2, number / 4, number % 3)   # 7, 8, 9, 10
show(number > 5, number == 10, number != 3)             # 11 compare
when number > 5:                               # 12 when
    show("big")
otherwise:
    show("small")
total = 0
each i within span(10):                          # 13 repeat
    when i == 2:
        skip                             # 15 skip
    when i == 6:
        stop                                # 14 stop
    total += i
show("total", total)
task square(n):
    give n * n                             # 16 return
show(square(7))                             # 17 call
scores = [80, 65, 90, 72]
scores.push(95)                            # 21 append
scores.arrange()                                # 18 sort
show(scores)
show(scores.locate(90), scores[0], scores[-1])   # 19 find, 20 at
scores.erase(65)                            # 22 remove
scores.flip()                             # 24 reverse
show(scores, size(scores))                   # 23 count
show(whole("123") + 1, decimal("2.5"), text(12) + "!", truth(0))   # 25 convert
