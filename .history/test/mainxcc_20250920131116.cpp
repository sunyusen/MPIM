#include<bits/stdc++.h>
using namespace std;
int dfs(int now, int pre, vector<vector<int>>& adj, vector<int>& memory){
	if(memory[now]!=0) return memory[now];
	int res = 0;
	for(int i = 0; i < adj[now].size(); i++){
		res = max(res, dfs());
	}
	memory[now] = res;
	return res;
}
int f(vector<int>& arr, vector<vector<int>>& route){
	int n = arr.size();
	int m = route.size();
	vector<vector<int>> adj(n);
	for(int i = 0; i < route.size(); i++){
		adj[route[i][0]-1].push_back(route[i][1]-1);
	}
	int res = 0;
	vector<int> memory(n,0);
	for(int i = 0; i < m; i++){
		res = max(res, dfs(i, -1, adj, memory));
	}
	return res;
}

int main(){
}