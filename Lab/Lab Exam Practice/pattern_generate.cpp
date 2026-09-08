#include <iostream>
#include <regex>
using namespace std;

int main() {
    string pattern;
    cin >> pattern;

    regex re(pattern);

    string alphabet = "ab";

    for (int len = 0; len <= 6; len++) {

        int total = 1;

        for (int i = 0; i < len; i++)
            total *= 2;

        for (int num = 0; num < total; num++) {

            string s = "";

            int x = num;//0

            for (int i = 0; i < len; i++) {
                s += alphabet[x % 2];
                x /= 2;
            }

            if (regex_match(s, re)) {
                if (s == "")
                    cout << "empty" << endl;
                else
                    cout << s << endl;
            }
        }
    }

    return 0;
}
