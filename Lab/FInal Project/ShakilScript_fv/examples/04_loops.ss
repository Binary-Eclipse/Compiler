# 04 - Loops: for, while, break, continue
for i in range(5):
    print("Hello", i)

total = 0
for n in range(1, 11):
    total += n
print("Sum 1..10 =", total)

count = 0
while count < 10:
    count += 1
    if count % 2 == 0:
        continue          # skip even numbers
    if count == 9:
        break             # stop the loop
    print("odd:", count)

for letter in "abc":
    print(letter)
