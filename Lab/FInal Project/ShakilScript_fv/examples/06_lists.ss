# 06 - Lists: sort, find, access, append, remove, count, reverse
numbers = [5, 3, 1, 4, 2]
print(numbers)

numbers.append(9)             # add an item
print(numbers)
numbers.sort()                # sort in place
print(numbers)
print("index of 4:", numbers.index(4))   # find
print("first:", numbers[0], "last:", numbers[-1])   # access
numbers.remove(9)             # remove a value
print(numbers, "count:", len(numbers))   # count items
numbers.reverse()
print(numbers)
print(sorted([3, 1, 2], reverse=True))
print(numbers[1:3], numbers[::-1])
print(3 in numbers, 42 in numbers)
