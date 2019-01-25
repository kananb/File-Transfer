#include "csocket.h"
#include <sstream>
#include <algorithm>
#include <iterator>

#pragma comment(lib, "Ws2_32.lib")


csocket::csocket(const std::string& ip, const std::string& port) :
	ip(ip), port(port), sock(INVALID_SOCKET), result(NULL), hints(), wsaData(), i_result(0) {
}



int csocket::init() {
	i_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (i_result != 0) {
		printf("WSAStartup failed: %d\n", i_result);
		return 1;
	}


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	i_result = getaddrinfo(ip.c_str(), port.c_str(), &hints, &result);

	if (i_result != 0) {
		std::cerr << "getaddrinfo falied: " << i_result << "\n";
		WSACleanup();
		return 1;
	}


	sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (sock == INVALID_SOCKET) {
		std::cerr << "socket failed: " << WSAGetLastError() << "\n";
		WSACleanup();
		return 1;
	}


	return 0;
}


int csocket::connectsocket() {
	if (sock == INVALID_SOCKET) {
		std::cerr << "socket not initialized\n";
		return 1;
	}


	i_result = connect(sock, result->ai_addr, (int)result->ai_addrlen);

	if (i_result == SOCKET_ERROR) {
		std::cerr << "connection failed: " << WSAGetLastError() << "\n";
		closesocket(sock);
		sock = INVALID_SOCKET;
	}


	freeaddrinfo(result);
	if (sock == INVALID_SOCKET) {
		WSACleanup();
		return 1;
	}


	return 0;
}


void csocket::disconnectsocket() {
	shutdown(sock, SD_BOTH);
	closesocket(sock);
	WSACleanup();
}


bool csocket::isconnected() {
	char error;
	socklen_t len = sizeof(error);
	getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);

	return !error;
}



int csocket::sendout(const File& file, bool showOut, int buflen) {
	return sendout(file.contents(), showOut, buflen);
}

int csocket::sendout(const std::vector<char>& data, bool showOut, int buflen) {
	int bytes = (int)data.size();
	int progress = 0;

	if (buflen <= 0 || buflen > bytes) {
		buflen = bytes;
	}

	do {
		if (progress + buflen > bytes) buflen = bytes - progress;
		i_result = send(sock, &data[0] + progress, buflen, 0);

		if (i_result == SOCKET_ERROR) {
			std::cerr << "send failed: " << i_result << "\n";
			closesocket(sock);
			WSACleanup();
			return -1;
		}

		progress += i_result;
		if (showOut) std::cout << "bytes sent: " << progress << "/" << bytes << "\n";
	} while (progress < bytes);

	return progress;
}


int csocket::sendout(const std::string& data, bool showOut, int buflen) {
	return sendout(data.c_str(), showOut, buflen);
}


int csocket::sendout(const char* data, bool showOut, int buflen) {
	int bytes = (int)strlen(data);
	int progress = 0;

	if (buflen <= 0 || buflen > bytes) {
		buflen = bytes;
	}

	do {
		if (progress + buflen > bytes) buflen = bytes - progress;
		i_result = send(sock, data + progress, buflen, 0);

		if (i_result == SOCKET_ERROR) {
			std::cerr << "send failed: " << i_result << "\n";
			closesocket(sock);
			WSACleanup();
			return -1;
		}

		progress += i_result;
		if (showOut) std::cout << "bytes sent: " << progress << "/" << bytes << "\n";
	} while (progress < bytes);

	return progress;
}



int csocket::receive(std::vector<char>& storage, bool showOut, int buflen) {
	if (buflen <= 0) buflen = DEFAULT_BUFLEN;
	storage.clear();
	storage.reserve(buflen);
	return receive(&storage[0], showOut);
}

int csocket::receive(std::string& storage, bool showOut, int buflen) {
	if (buflen <= 0) buflen = DEFAULT_BUFLEN;
	storage.clear();
	storage.reserve(buflen);
	return receive(&storage[0], showOut);
}

int csocket::receive(char* storage, bool showOut) {
	i_result = recv(sock, storage, sizeof(storage), 0);

	if (!showOut) return i_result;
	if (i_result > 0) {
		std::cout << "bytes received: " << i_result << "\n";
	}
	else if (i_result == 0) {
		std::cout << "connection closed\n";
	}
	else {
		std::cerr << "recv failed: " << WSAGetLastError() << "\n";
	}


	return i_result;
}



const std::string& csocket::getPort() const {
	return port;
}

const std::string& csocket::getIp() const {
	return ip;
}


void csocket::setPort(const std::string& port) {
	this->port = port;
}

void csocket::setIp(const std::string& ip) {
	this->ip = ip;
}