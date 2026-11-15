s = "Hello, ShakilScript"
print(s, len(s), s[0], s[-1], s[7:], s[:5], s[::-1], s[::2])
print(s.upper(), s.lower(), "  pad  ".strip() + "|", "xxhixx".strip("x"))
print(s.find("Shakil"), s.find("zzz"), s.count("l"), "abab".count("ab"), "abc".count(""))
print("a-b-c".split("-"), "  a  b   c ".split(), "abc".split("b"), "".split("x"), "a,b,".split(","))
print("-".join(["a", "b", "c"]), "".join(["x", "y"]), ", ".join([]))
print("banana".replace("a", "o"), "aaa".replace("a", "bb"), "abc".replace("", "-"), "abc".replace("x", "y"))
print("abc".startswith("ab"), "abc".endswith("bc"), "abc".startswith("bc"))
print("ab" + "cd", "ab" * 3, 3 * "ab", "ab" * 0, "ab" * -1)
print("a" < "b", "apple" < "apricot", "Z" < "a", "abc" == "abc", "abc" != "abd")
print("ell" in "hello", "xyz" in "hello", "" in "x", "q" not in "abc")
print("it's", 'say "hi"', "back\\slash", "new\nline", 'both \' and "')
print(["it's", 'say "hi"', "plain", "tab\t", "nl\n"])
print(str(5) + str(2.5) + str(True) + str(None) + str([1, "a"]))
print("5" + "5", int("5") + int("5"), "5" * 3)
print("abc"[1], "abc"[-1], "abc"[0:2], "abc"[5:], "abc"[1:100])
count = 0
for ch in "mississippi":
    if ch == "s":
        count += 1
print(count)
def is_palindrome(t):
    return t == t[::-1]
print(is_palindrome("level"), is_palindrome("hello"))
word = ""
for ch in "reverse me":
    word = ch + word
print(word)
print(bool(""), bool(" "), "" or "default", "x" and "y")
vowels = 0
for ch in "Programming Language".lower():
    if ch in "aeiou":
        vowels += 1
print(vowels)
