#include<bits\stdc++.h>
using namespace std;
int main(){
cout<<"Enter identifier:"<<endl;
string id;
cin>>id;
bool valid=false;

set<string> keywords={
"int","float","double","char","bool","if","else","for","while"

};
if(keywords.count(id)){
    valid=false;
}else{
 if (isalpha(id[0]) || id[0]=='_'){

    for(int i=1;i<id.length();i++){
        if(!isalnum(id[i])&& id[i]!='_'){
            valid=false;
            break;
        }else{
            valid=true;
        }
    }
    }
}
if(valid){
    cout<<"Valid identifier"<<endl;
}else{
   cout<<"invalid identifier"<<endl;
}
return 0;
}
