#ifndef GUARD_CSOCKET_H
#define GUARD_CSOCKET_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <iostream>

#include "File.h"


class csocket {
public:
	csocket(const std::string& = "127.0.0.1", const std::string& = "12345");


	int init();

	int connectsocket();
	void disconnectsocket();

	bool isconnected();


	int sendout(const File&, bool = 1, int = -1);
	int sendout(const std::vector<char>&, bool = 1, int = -1);
	int sendout(const std::string&, bool = 1, int = -1);
	int sendout(const char*, bool = 1, int = -1);


	int receive(std::vector<char>&, bool = 1, int = DEFAULT_BUFLEN);
	int receive(std::string&, bool = 1, int = DEFAULT_BUFLEN);
	int receive(char*, bool = 1);


	const std::string& getPort() const;
	const std::string& getIp() const;

	void setPort(const std::string&);
	void setIp(const std::string&);


	SOCKET& getSocket() {
		return sock;
	}

private:
	const static int DEFAULT_BUFLEN = 134217728; //1024*1024*128

	std::string ip, port;

	SOCKET sock;
	addrinfo *result = NULL, hints;
	WSADATA wsaData;
	int i_result;
};


#endif //GUARD_CSOCKET_H