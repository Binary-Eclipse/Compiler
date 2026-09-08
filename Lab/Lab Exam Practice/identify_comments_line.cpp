#include<bits/stdc++.h>
using namespace std;

int main(){
string s;
getline(cin,s);

//length
int n=s.length();
cout<<"the length: "<<n<<endl;


if(n>=2 && s[0]=='/' && s[1]=='/'){
    cout<<"This is Single Line Comment"<<endl;
}
else if(n>=4 &&
        s[0]=='/' &&
        s[1]=='*' &&
        s[n-2]=='*'&&
        s[n-1]=='/'
        ){
            cout<<"This is multi-line comments"<<endl;
        }
else {
    cout<<"this is not a comment"<<endl;
}
return 0;

}
