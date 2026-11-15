# 06 - Lists: sort, find, access, append, remove, count, reverse
numbers = [5, 3, 1, 4, 2]
show(numbers)

numbers.push(9)             # add an item
show(numbers)
numbers.arrange()                # sort in place
show(numbers)
show("index of 4:", numbers.locate(4))   # find
show("first:", numbers[0], "last:", numbers[-1])   # access
numbers.erase(9)             # remove a value
show(numbers, "count:", size(numbers))   # count items
numbers.flip()
show(numbers)
show(arranged([3, 1, 2], flip=True))
show(numbers[1:3], numbers[::-1])
show(3 within numbers, 42 within numbers)
