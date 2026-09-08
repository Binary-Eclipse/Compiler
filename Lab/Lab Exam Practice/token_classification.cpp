#include <bits/stdc++.h>
using namespace std;

int main() {
    string s;
    getline(cin, s);

    set<string> keywords = {
        "int", "float", "double", "char",
        "bool", "if", "else", "for", "while",
        "return"
    };

    for (int i = 0; i < s.length(); i++) {

        // Keyword or Identifier
        if (isalpha(s[i]) || s[i] == '_') {

            string w = "";

            while (i < s.length() &&
                   (isalnum(s[i]) || s[i] == '_')) {
                w += s[i];
                i++;
            }

            if (keywords.count(w))
                cout << w << " -> Keyword" << endl;
            else
                cout << w << " -> Identifier" << endl;
        }

        // Constant
        else if (isdigit(s[i])) {

            string num = "";

            while (i < s.length() && isdigit(s[i])) {
                num += s[i];
                i++;
            }

            cout << num << " -> Constant" << endl;
        }

        // Operators
        else if (s[i] == '+' || s[i] == '-' ||
                 s[i] == '*' || s[i] == '/') {

            cout << s[i] << " -> Arithmetic Operator" << endl;
        }

        else if (s[i] == '=') {
            cout << s[i] << " -> Assignment Operator" << endl;
        }

        // Special symbol
        else if (s[i] == ';' || s[i] == ',' ||
                 s[i] == '(' || s[i] == ')') {

            cout << s[i] << " -> Special Symbol" << endl;
        }
    }

    return 0;
}
