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
		if(now == m) return arr[now];
		if(now+1<MAX && arr[now+1] == -1){ arr[now+1] = arr[now]+1; que.push(now+1); }
		if(now-1>=0 && arr[now-1] == -1){ arr[now-1] = arr[now]+1; que.push(now-1); }
		if(now*2<MAX && arr[now*2] == -1){ arr[now*2] = arr[now]+1; que.push(now*2); }
	}
	return -1;
}