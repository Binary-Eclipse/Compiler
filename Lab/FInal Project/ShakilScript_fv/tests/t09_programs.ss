# small complete programs
def is_prime(n):
    if n < 2:
        return False
    i = 2
    while i * i <= n:
        if n % i == 0:
            return False
        i += 1
    return True

primes = []
for n in range(2, 40):
    if is_prime(n):
        primes.append(n)
print(primes)

def bubble_sort(items):
    n = len(items)
    for i in range(n):
        for j in range(n - i - 1):
            if items[j] > items[j + 1]:
                tmp = items[j]
                items[j] = items[j + 1]
                items[j + 1] = tmp
    return items
print(bubble_sort([64, 34, 25, 12, 22, 11, 90]))

def gcd(a, b):
    while b != 0:
        t = b
        b = a % b
        a = t
    return a
print(gcd(48, 18), gcd(17, 5))

def fib_list(count):
    seq = [0, 1]
    while len(seq) < count:
        seq.append(seq[-1] + seq[-2])
    return seq
print(fib_list(15))

def binary_search(items, target):
    low = 0
    high = len(items) - 1
    while low <= high:
        mid = (low + high) // 2
        if items[mid] == target:
            return mid
        elif items[mid] < target:
            low = mid + 1
        else:
            high = mid - 1
    return -1
data = [2, 5, 8, 12, 16, 23, 38, 56, 72, 91]
print(binary_search(data, 23), binary_search(data, 7))

def average(values):
    return sum(values) / len(values)
marks = [78, 85, 92, 66]
print("average:", average(marks), "max:", max(marks), "min:", min(marks))

def table(n):
    for i in range(1, 6):
        print(n, "x", i, "=", n * i)
table(7)

def count_words(sentence):
    words = sentence.split()
    return len(words)
print(count_words("the quick brown fox"))

celsius = [0, 37, 100, -40]
for c in celsius:
    print(c, "C =", c * 9 / 5 + 32, "F")

def reverse_words(sentence):
    parts = sentence.split(" ")
    parts.reverse()
    return " ".join(parts)
print(reverse_words("one two three"))

def digits_sum(n):
    total = 0
    for ch in str(n):
        total += int(ch)
    return total
print(digits_sum(98765))

stars = ""
for i in range(1, 5):
    stars += "*"
    print(stars)
