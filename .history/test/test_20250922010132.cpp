#include<bits/stdc++.h>
using namespace std;

string f1(vector<int>& arr){

    for(int i = 0; i < arr.size(); i++){
        ans += to_string(arr[i]);
    }
    return ans;
}

string f2(vector<int>& arr){
    string ans = "";
    for(int i = 0; i < arr.size(); i++){
        ans += to_string(arr[i]);
    }
    return ans;
}

}

int main(){
}