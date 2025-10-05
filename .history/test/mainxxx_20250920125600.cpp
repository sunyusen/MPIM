#include<bit/stdc++.h>
using namespace std;

int min_change(vector<int>& arr){
	int n = arr.size();
	int m = arr.size()/2;
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
	for(int i = 1; i < m; i++){
		
	}
	return res;
}
int main(){

}