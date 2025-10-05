#include<iostream>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<cstring>
#define PORT 8080
using namespace std;

void f1(){


}

int main(){
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd==-1) {
		cerr << "socket failed" << endl;
		return -1;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); //指定连接的ip地址
	
	if(connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0){
		cerr << "connect failed" << endl;
		close(fd);
		return -1;
	}
	cout << "Connected to server on port " << PORT << endl;

	const char *hello = "Hello from client";
	send(fd, hello, strlen(hello), 0);
	cout << "Message sent to server: " << hello << endl;

	char buffer[1024] = {0};
	int valread = read(fd, buffer, 1024);
	cout << "server says: " << buffer << endl;
	
	close(fd);
	return 0;
}