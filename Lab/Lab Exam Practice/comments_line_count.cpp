#include<bits\stdc++.h>
using namespace std;
int main(){
string s;
int single_count=0;
int multi_count=0;
bool multi=false;

while(getline(cin,s)){
    if(s=="END"){
        break;
    }
    int n=s.length();

    if (multi){
        multi_count++;
        if (n>=2 && s[n-2]=='*' &&s[n-1]=='/'){
            multi=false;
        }
    }

    else if ( n>=2 && s[0]=='/' && s[1]=='/'){
        single_count++;
    }
    else if( n>=2 && s[0] == '/' && s[1]=='*'){
        multi_count++;
        if (n>=2 && s[n-2]=='*' &&s[n-1]=='/'){
            multi=false;
        }else{
            multi=true;
        }
    }

}

cout<<"Single comments line: "<<single_count<<endl;
cout<<"Multi comments line: "<<multi_count<<endl;


return 0;
}
