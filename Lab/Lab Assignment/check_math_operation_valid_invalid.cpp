#include <iostream>
#include <string>
#include <cctype>
using namespace std;

string s;
int pos = 0;

void skipSpaces() {
    while (pos < s.length() && isspace(s[pos]))
        pos++;
}

// F -> numbers | identifiers
bool F() {
    skipSpaces();

    if (pos >= s.length())
        return false;

    // Accept numbers: 123
    if (isdigit(s[pos])) {
        while (pos < s.length() && isdigit(s[pos]))
            pos++;
        return true;
    }

    // Accept strings/variables: abc, x, value123
    if (isalpha(s[pos])) {
        while (pos < s.length() &&
               isalnum(s[pos]))
            pos++;
        return true;
    }

    return false;
}

// T -> T * F | T / F | F
bool T() {
    if (!F())
        return false;

    while (true) {
        skipSpaces();

        if (pos < s.length() &&
            (s[pos] == '*' || s[pos] == '/')) {
            pos++;

            if (!F())
                return false;
        } else {
            break;
        }
    }

    return true;
}

// E -> E + T | E - T | T
bool E() {
    if (!T())
        return false;

    while (true) {
        skipSpaces();

        if (pos < s.length() &&
            (s[pos] == '+' || s[pos] == '-')) {
            pos++;

            if (!T())
                return false;
        } else {
            break;
        }
    }

    return true;
}

int main() {
    cout << "Enter a mathematical statement: ";
    getline(cin, s);

    pos = 0;

    if (E()) {
        skipSpaces();

        if (pos == s.length())
            cout << "Valid Statement" << endl;
        else
            cout << "Invalid Statement" << endl;
    } else {
        cout << "Invalid Statement" << endl;
    }

    return 0;
}
