#include <iostream>
#include <regex>
#include <string>
#include <queue>
#include <set>

using namespace std;

int main() {
    string pattern;

    cout << "Enter regular expression: ";
    cin >> pattern;

    regex re(pattern);

    queue<string> q;
    set<string> visited;

    q.push("");
    visited.insert("");

    int count = 0;
    const int MAX_OUTPUT = 15;
    const int MAX_LENGTH = 6;


    string alphabet = "ab";

    cout << "\nGenerated strings:\n";

    while (!q.empty() && count < MAX_OUTPUT) {
        string current = q.front();
        q.pop();

        if (regex_match(current, re)) {
            if (current.empty())
                cout << "empty\n";
            else
                cout << current << "\n";

            count++;
        }


        if (current.length() == MAX_LENGTH)
            continue;

        for (char c : alphabet) {
            string next = current + c;

            if (visited.find(next) == visited.end()) {
                visited.insert(next);
                q.push(next);
            }
        }
    }

    return 0;
}
