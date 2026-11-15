# 03 - Decisions: if / elif / else
score = 72

when score >= 90:
    show("Grade A")
orwhen score >= 80:
    show("Grade B")
orwhen score >= 70:
    show("Grade C")
otherwise:
    show("Grade F")

age = 20
has_id = True
when age >= 18 and has_id:
    show("Entry allowed")
when not (age < 18):
    show("You are an adult")
show(age == 20, age != 20, age > 30, age <= 20)
