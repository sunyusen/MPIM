#include<bits/stdc++.h>
using namespace std;

vector<vector<int>> around = {{-1,+1},{-1,0},{-1,-1},{0,-1},{0,+1},{+1,-1},{+1,0},{+1,+1}};
unordered_map<string, int> cnt;	//head对应的个数为

string getHead(string key, unordered_map<string, string>& umap){
	if(umap[key] == key) return key;
	string tmp = getHead(umap[key], umap);
	umap[key] = tmp;
	return tmp;
}

void kset(int &total, vector<int>& tk, unordered_map<string, string>& umap){
	string key = to_string(tk[0])+"."+to_string(tk[1]);
	set<string> hasset;	//已经遍历
	umap[key] = key;
	int newtotal = 1;
	for(int i = 0; i < around.size(); i++){
		string tkey = to_string(tk[0]+around[i][0])+"."+to_string(tk[1]+around[i][1]);
		if(umap.find(tkey) == umap.end()) continue;
		string head = getHead(tkey, umap);	//获取他的头头
		if(hasset.find(head) != hasset.end()) continue;	//已经处理过了
		//还没有处理过
		hasset.insert(head);
		total -= cnt[head]*cnt[head];	//从总数里面减去
		newtotal += cnt;
		umap[head] = key;				//加入当前的并查集，且从之前的数量里面删除
		cnt.erase(head);
	}
	cnt[key] = newtotal;
	total += newtotal*newtotal;
}

void f1(vector<vector<int>>& arr){
	unordered_map<string, string> umap;
	int total = 0;
	for(int i = 0; i < arr.size(); i++){
		kset(total,arr[i],umap);
		cout << total << endl;
	}
}

int main(){

}