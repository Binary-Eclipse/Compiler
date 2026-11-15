# 03 - Decisions: if / elif / else
score = 72

if score >= 90:
    print("Grade A")
elif score >= 80:
    print("Grade B")
elif score >= 70:
    print("Grade C")
else:
    print("Grade F")

age = 20
has_id = True
if age >= 18 and has_id:
    print("Entry allowed")
if not (age < 18):
    print("You are an adult")
print(age == 20, age != 20, age > 30, age <= 20)
