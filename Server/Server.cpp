//#include "pch.h"
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <iostream>
#include <winsock2.h>
#include <thread>
#include <regex>
#pragma comment(lib,"Ws2_32.lib")

void serverclient(SOCKET talksocket);
std::string fileposition("C:\\Users\\梅朝瑞\\OneDrive\\工程\\计算机网络实验\\Server");

int main(void){
	//初始化winsock
	WSADATA wasdata;
	int re = WSAStartup(0x0202, &wasdata);//返回值

	if (re){
		std::cout << "WSAstartup error\n";
		return 0;
	}
	std::cout << "WSAstartup success\n";
	if (wasdata.wVersion != 0x0202){
		std::cout << "Version error\n";
		WSACleanup();
		return 0;
	}
	std::cout << "Version pass\n";
	//创建一个监听socket
	SOCKET tcpsocket;//服务器监听socket
	tcpsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (tcpsocket == INVALID_SOCKET){
		std::cout << "Get socket error\n";
		return 0;
	}
	std::cout << "get correct socket\n";
	//bind 监听socket

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	 bind(tcpsocket, (LPSOCKADDR)&addr, sizeof(addr));

	//监听socket，等待客户机的连接请求
	re = listen(tcpsocket, 100);//最多接收100个连接
	if (re == SOCKET_ERROR){
		std::cout << "listen error\n";
		return 0;
	}
	std::cout << "listen correct\n";
	//accept接收客户机请求 连接并生成会话socket

	while (1)
	{
		int sockerror;
		int length=sizeof(sockaddr);
		sockaddr_in clientaddr;
		SOCKET talksocket = accept(tcpsocket, (sockaddr *)&clientaddr, &length);
		if (talksocket == INVALID_SOCKET){
			std::cout << "accept error\n";
			sockerror = WSAGetLastError();
			return 0;
		}
		else{
			std::cout << "accept success\n";
			std::thread talkthread(serverclient, talksocket);//开启一个新的线程来处理这一个客户的请求
			std::cout << "request IP:" << (int)clientaddr.sin_addr.S_un.S_un_b.s_b1 << "." << (int)clientaddr.sin_addr.S_un.S_un_b.s_b2 << "." << (int)clientaddr.sin_addr.S_un.S_un_b.s_b3 << "." << (int)clientaddr.sin_addr.S_un.S_un_b.s_b4;
			std::cout << "\nrequest Port:" << clientaddr.sin_port << "\n";
			talkthread.detach();
		}
	}
	return 0;
}

void serverclient(SOCKET talksocket){//进行会话的函数。
	int sendre;
	char* buf = (char*)malloc(1024 * sizeof(char));
	int len = 1024;
	int recharnum;
	const char* NOTFOUND = "HTTP/1.1 404 Not Found\r\n";
	const char* REQERROR = "HTTP/1.1 400 Bad Request\r\n";
	const char* FORMERROR = "HTTP/1.1 400 Bad Request\r\n";
	const char* LOADERROR = "HTTP/1.1 400 Bad Request\r\n";
	recharnum = recv(talksocket, buf, len, 0);
	if (recharnum == SOCKET_ERROR){
		recharnum = WSAGetLastError();
		std::cout << "thread falied!";
		std::cout << std::endl;
		return;
	}
	else{
		//对接受到的消息进行解析
		std::smatch strmatch;//正则表达式结果文本
		std::regex regulation("([A-Za-z]+) +(.*) +(HTTP/[0-9][.][0-9])");//正则表达式规则，，匹配请求报文的请求行
		std::string str(buf);//需要用正则表达式的原始文本

		int matchnum = std::regex_search(str, strmatch, regulation);
		if (matchnum == 0){
			std::cout << "request message exception\n";
			sendre = send(talksocket, REQERROR, strlen(REQERROR),0);
			closesocket(talksocket);
			return;
		}
		else{//分离 GET  url http_version
			std::string msg_get = strmatch[1];
			std::string msg_url = strmatch[2];
			std::smatch filetype;
			std::regex regulation2("\\..*");
			matchnum = regex_search(msg_url, filetype, regulation2);
			if (matchnum == 0){
				std::cout << msg_get + msg_url+"FORMAT ERROR\n";
				sendre=send(talksocket, FORMERROR, strlen(FORMERROR),0);
				closesocket(talksocket);
				return;
			}
			else
			{
				std::ifstream f(fileposition + msg_url,std::ios::binary);
				if (!f){//没有找到文件
					std::cout << msg_url + "NOT FOUND";
					//sendre=send(talksocket, NOTFOUND, strlen(NOTFOUND),0);
					std::ifstream f(fileposition + "404.html", std::ios::binary);
					goto sendfile;
					closesocket(talksocket);
					return;
				}
				else{//如果找到了对应的文件
					sendfile:
					std::filebuf* tmp = f.rdbuf();
					int size = tmp->pubseekoff(0, f.end,f.in);
					tmp->pubseekpos(0, f.in);
					if (size <= 0){
						std::cout << "load file into memory failed!\n";
						sendre=send(talksocket, LOADERROR, strlen(LOADERROR), 0);
						closesocket(talksocket);
						return;
					}
					else{
						std::string Content_Type="image/jpg";
						char* buffer = new char[size];
						char* tail = buffer + size;
						tmp->sgetn(buffer,size);
						f.close();
						std::cout << "success return file " + msg_url;
						std::cout << std::endl;
						std::stringstream remsg;
						remsg << "HTTP/1.1 200 OK\r\n" << "Connection:close\r\n" << "Server:Macyrate\r\n"<<"Content Length:"<<size
							<<"\r\n"<<"Content Type:"+Content_Type<<"\r\n\r\n";
						std::string remsgstr = remsg.str();
						const char* remsgchar = remsgstr.c_str();
						int tmpsize = strlen(remsgchar);
						sendre=send(talksocket, remsgchar, tmpsize, 0);
						while (buffer < tail) {
							sendre = send(talksocket, buffer, size, 0);
							buffer = buffer + sendre;
							size = size - sendre;
						}
						closesocket(talksocket);
						return;
					}
				}
			}
		}
	}
	return;
}