#include<bit/stdc++.h>
using namespace std;

int min_change(vector<int>& arr){
	int n = arr.size();
	int m = arr.size()/2;
	if()
	//首先初始化cost数组:从cost[i][j]表示从第m块的初始状态变成j的代价
	vector<vector<int>> cost(m, vector<int>(10,0));
	for(int i = 0; i < m; i++){
		int a = arr[i*2], b = arr[i*2+1];
		for(int j = 1; j < 10; j++){
			cost[i][j] = (a!=j) + (b!=j); 
		}
	}

	//dp数组:dp[i][j]表示前i块成立，且第i块变成j的总代价，
	vector<vector<int>> dp(m, vector<int>(10,0));
	for(int i = 0; i < 10; i++) dp[0][i] = cost[0][i];
	int res = INT_MAX;
	for(int i = 1; i < m; i++){
		for(int j = 1; j < 10; j++){
			dp[i][j] = INT_MAX;
			for(int k = 1; k < 10; k++){
				if(j==k) continue;
				dp[i][j] = min(dp[i][j], dp[i-1][k] + cost[i][j]);
			}
			if(i==m-1) res = min(res, dp[i][j]);
		}
	}
	return res;
}
int main(){

}