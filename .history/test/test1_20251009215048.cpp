#include <bits/stdc++.h>
#define MAX 100000
using namespace std;

int f(int n, int m){
	vector<int> arr(MAX, -1);
	arr[n] = 0;
	queue<int> que;
	que.push(n);
	while(!que.empty()){
		int now = que.front();
		que.pop();
		if(now+1<100000 arr[now+1] == -1){{ arr[now+1] = arr[now]+1; que.push(now+1); }
	}
}