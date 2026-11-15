# 04 - Loops: for, while, break, continue
each i within span(5):
    show("Hello", i)

total = 0
each n within span(1, 11):
    total += n
show("Sum 1..10 =", total)

tally = 0
during tally < 10:
    tally += 1
    when tally % 2 == 0:
        skip          # skip even numbers
    when tally == 9:
        stop             # stop the loop
    show("odd:", tally)

each letter within "abc":
    show(letter)
