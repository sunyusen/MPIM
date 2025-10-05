#include<bits/stdc++.h>
using namespace std;

string f1(vector<int>& arr){
	sort(arr.begin(), arr.end(), [](int a,int b){
		string sa = to_string(a);
		string sb = to_string(b);
		return sa + sb > sb + sa;
	});
	string ans = "";
	for(int i = 0; i < arr.size(); i++){
		ans += to_string(arr[i]);
	}
	return ans;
}

int main(){
}