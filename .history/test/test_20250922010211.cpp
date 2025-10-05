#include<bits/stdc++.h>
using namespace std;

string f1(vector<int>& arr){
	sort(arr.begin(), arr.end(), [](int a,int b){
		string sa = to_string(a);
		string sb = to_string(b);
		return sa + sb > sb + sa;
	});

}

int main(){
}