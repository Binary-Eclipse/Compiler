#include<bits\stdc++.h>
using namespace std;
int main(){
cout<<"Enter identifier:"<<endl;
string id;
getline(cin,id);
int c=0;

set<string> keywords={
"int","float","double","char","bool","if","else","for","while"

};
for(int i=0; i<id.length();i++){

    if(isalpha(id[i])|| id[i]=='_'){
        string w="";
        while(i<id.length()&& (isalnum(id[i]) || id[i]=='_')){
            w+=id[i];
            i++;
        }
        if(!keywords.count(w)){
                c++;
            cout<<"identifier "<<c<<":"<<w<<endl;
        }
    }
}
return 0;
}
