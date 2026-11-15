// ShakilScript functions 11-25 demonstration

// 11. compare()
print(compare(10, ">", 5));

// 12. when()
when(compare(10, ">", 5)) {
    print("The condition is true.");
}

// 13-15. repeat(), skip(), stop()
create("i", 0);
repeat(6) {
    update("i", add(get("i"), 1));
    when(compare(get("i"), "==", 2)) {
        skip();
    }
    when(compare(get("i"), "==", 5)) {
        stop();
    }
    print(get("i"));
}

// 16-17. return(), function definition, call()
function square(number) {
    return(multiply(get("number"), get("number")));
}
print(call("square", 7));

// 18-24. Collection functions. Indexes start at zero.
create("numbers", [8, 3, 5, 1]);
print(count(get("numbers")));
print(find(get("numbers"), 5));
print(at(get("numbers"), 1));
append(get("numbers"), 9);
print(get("numbers"));
sort(get("numbers"));
print(get("numbers"));
reverse(get("numbers"));
print(get("numbers"));
remove(get("numbers"), 0);
print(get("numbers"));

// 25. convert()
print(convert("123", "int"));
print(convert(12, "string"));
print(convert("true", "bool"));
