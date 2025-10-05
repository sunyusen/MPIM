#include<bits/stdc++.h>
using namespace std;
int dfs(int now, int pre, vector<vector<int>>& route, vector<int>& memory){
	if(memory[now]!=0) return memory[now];
	int res = 0;
	for(int i = 0; i < )
}
int f(vector<int>& arr, vector<vector<int>>& route){
	int n = arr.size();
	int m = route.size();
	vector
	int res = 0;
	vector<int> memory(n,0);
	for(int i = 0; i < m; i++){
		res = max(res, dfs(i, -1, route));
	}
	return res;
}

int main(){
}