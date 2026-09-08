
#include<bits\stdc++.h>
using namespace std;
int main(){
cout<<"Press 'e' when finish taking input "<<endl;
string s;
int single_count=0;
int multi_count=0;
bool multi=false;
int c=0;
while(getline(cin,s)){
    if(s=="e"){
        break;
    }
 int n=s.length();
 for(int i=0;i<n;i++){
    if (multi){

        if (s[i]=='*' &&i+1<n &&s[i+1]=='/'){
            multi=false;
            i++;
        }else{
          c++;
        }
    }

    else if (s[i]=='/' && i+1<n && s[i+1]=='/'){
           i+=2;
           while(i<n){
            c++;
            i++;
           }
    }
    else if(s[i]=='/' && i+1<n && s[i+1]=='*'){
       multi=true;
       i++;
    }

}
}
cout<<"total characters: "<<c<<endl;


return 0;
}
