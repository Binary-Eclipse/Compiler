#include <bits/stdc++.h>
using namespace std;

int main() {
    string s;
    getline(cin, s);

    set<string> keywords = {
        "int", "float", "double", "char",
        "bool", "if", "else", "for", "while"
    };

    int keyword = 0;
    int identifier = 0;
    int constant = 0;
    int op = 0;

    for (int i = 0; i < s.length(); i++) {

        // Word
        if (isalpha(s[i]) || s[i] == '_') {

            string w = "";

            while (i < s.length() &&
                   (isalnum(s[i]) || s[i] == '_')) {
                w += s[i];
                i++;
            }

            if (keywords.count(w))
                keyword++;
            else
                identifier++;
        }

        // Number
        else if (isdigit(s[i])) {
            constant++;

            while (i < s.length() && isdigit(s[i]))
                i++;
        }

        // Operator
        else if (s[i] == '+' || s[i] == '-' ||
                 s[i] == '*' || s[i] == '/' ||
                 s[i] == '=' || s[i] == '<' ||
                 s[i] == '>') {
            op++;
        }
    }

    cout << "Keywords: " << keyword << endl;
    cout << "Identifiers: " << identifier << endl;
    cout << "Constants: " << constant << endl;
    cout << "Operators: " << op << endl;

    return 0;
}
