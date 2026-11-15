# list operations: sort, find(index), at, append, remove, count(len), reverse
numbers = [8, 3, 5, 1]
print(numbers, len(numbers))
numbers.append(9)
print(numbers)
numbers.sort()
print(numbers)
numbers.reverse()
print(numbers)
print(numbers[0], numbers[-1], numbers[2])
print(numbers.index(5), 5 in numbers, 42 in numbers, 42 not in numbers)
numbers.remove(5)
print(numbers)
print(numbers.pop(), numbers.pop(0), numbers)
numbers.insert(1, 77)
numbers.insert(-1, 88)
numbers.insert(100, 5)
print(numbers)
numbers.extend([1, 2])
numbers.extend("ab")
print(numbers, numbers.count(1))
print(sorted([3, 1, 2]), sorted([3, 1, 2], reverse=True), sorted("hello"))
print(sorted([2.5, 1, True, 0]), sorted(["b", "a", "C"]))
a = [1, 2, 3, 4, 5, 6]
print(a[1:3], a[:2], a[4:], a[-2:], a[::2], a[::-1], a[5:1:-2], a[10:], a[:-1], a[-100:100])
b = a
b.append(7)
print(a, a == b, a is b, a is not [1])
c = a[:]
c.append(8)
print(a, c)
a[0] = 100
a[-1] += 5
print(a)
m = [[1, 2], [3, 4]]
m[1][0] = 30
print(m, m[1][0], len(m))
print([0] * 4, [1, 2] * 2, [1] + [2, 3], [[0] * 2] * 2)
print([] == [], [1, 2] == [1, 2], [1, 2] < [1, 3], [1, 2] < [1, 2, 0], [2] > [1, 9])
print(list("abc"), list([1, 2]), list(range(4)), list(range(10, 0, -3)), len(list()))
print(min(["b", "a"]), max(["b", "a"]), sum([[1], [2]], []))
total = 0
for v in [1, 2, 3]:
    total += v
print(total)
grid = []
for r in range(3):
    row = []
    for c in range(3):
        row.append(r * c)
    grid.append(row)
print(grid)
words = ["pear", "apple", "fig"]
words.sort()
print(words, words.index("fig"))
q = [1, 2, 3]
for v in q:
    if v == 1:
        q.append(4)
print(q)
print(bool([]), bool([0]), [] or "empty", [1] and "full")
