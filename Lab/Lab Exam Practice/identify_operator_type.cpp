
#include <bits/stdc++.h>
using namespace std;

int main() {
    string s;
    cin >> s;

    for (int i = 0; i < s.length(); i++) {

        if (s[i] == '+' || s[i] == '-' ||
            s[i] == '*' || s[i] == '/' ||
            s[i] == '%') {

            cout << s[i] << " = Arithmetic Operator";
        }

        else if (s[i] == '>' || s[i] == '<' ||
                 s[i] == '=') {

            cout << s[i] << " = Relational/Assignment Operator";
        }

        else if (s[i] == '&' || s[i] == '|') {

            cout << s[i] << " = Logical Operator";
        }
    }

    return 0;
}
