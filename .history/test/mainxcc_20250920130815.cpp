#include<bits/stdc++.h>
using namespace std;

int f(vector<int>& arr, vector<vector<int>>& route){
	int n = arr.size();
	int m = route.size();
	int res = 0;
	vector<int> memory(n,0);
	for(int i = 0; i < m; i++){
		res = max(res, dfs(i, -1, route));
	}
	return res;
}

int main(){
}