#include<iostream>
#include<netinet/in.h>
#include<unistd.h>	//这个是干什么的
#include<cstring>
#define PORT 8080

using namespace std;
int main(){
	//创建套接字
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd==-1) {
		cerr << "socket failed" << endl;
		return -1;
	}
	//绑定地址
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;	//监听所有网卡

	//绑定地址
	if(bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0){
		cerr << "bind failed" << endl;
		return -1;
	}

	//开始监听，最多允许5个等待连接
	if(listen(fd, 5) == -1){
		cerr << "Listen failed" << endl;
		return -1;
	}
	cout << "Server listening on port" << PORT << endl;

	//接收一个客户端连接
	int client_fd = accept(fd, nullptr, nullptr);
	if(client_fd < 0){
		cerr << "Accept failed" << endl;
		close(fd);
		return -1;
	}

	char buffer[1024] = {0};
	int valread = read(fd, buffer, 1024);
	cout << "Message from client: " << buffer << endl;

	//向客户端发送回应
	const char *hello = "Hello from server";
	send(client_fd, hello, strlen(hello), 0);

	//关闭连接
	return 0;
}