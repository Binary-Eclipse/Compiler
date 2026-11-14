// when, repeat, stop, skip, functions, collections and convert working together

when(compare(10, ">", 5)) {
    print("Ten is greater");
}

repeat(5) {
    skip();
    print("never printed");
}
repeat(10) {
    print("stops after one pass");
    stop();
}

function addNumbers(a, b) {
    return(add(a, b));
}
function square(number) {
    return(multiply(number, number));
}
print(call("addNumbers", 10, 20));
print(call("square", 5));
print(call("square", 10));

create("numbers", [5, 2, 8, 1, 3]);
sort(get("numbers"));
print(get("numbers"));
print(find(get("numbers"), 8));
print(at(get("numbers"), 0));
append(get("numbers"), 10);
print(get("numbers"));
remove(get("numbers"), 2);
print(get("numbers"));
print(count(get("numbers")));
reverse(get("numbers"));
print(get("numbers"));

print(convert("25", "integer"));
print(convert("3.50", "float"));
print(convert(100, "string"));
print(convert("true", "boolean"));
