#include "file.h"
#include "communicator.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <math.h>
#include <thread>
#include <mutex>
#include <direct.h>
#include <filesystem>

#pragma comment(lib, "Ws2_32.lib")


std::string dnloadpath;

SOCKET server = INVALID_SOCKET, client = INVALID_SOCKET;
addrinfo* result = NULL, hints;
WSADATA wsaData;
int i_result = 0;

const short MAX_CONNECTIONS = 10;
SOCKET connections[MAX_CONNECTIONS];
short count = 0;

std::mutex lock;


DWORD WINAPI run(void*);
void close(SOCKET&, short);



std::map<std::string, std::string> files;


const std::string SET_ALPHA = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string SET_ALPHA_NUM = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const std::string SET_ALL = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
const std::string CURRENT_SET = SET_ALPHA_NUM;

std::string genkey(const std::string & charset, size_t setlen) {
	std::string ret(8, 0);
	for (short i = 0; i < 8; ++i) {
		ret[i] = charset[rand() % setlen];
	}

	size_t ret_index = 0, set_index = charset.find(ret[ret_index]);
	char original = ret[ret_index];
	while (files.find(ret) != files.end()) {
		if (ret_index >= 8) return genkey(charset, setlen);
		ret[ret_index] = charset[++set_index];
		if (ret[ret_index] == original) {
			original = ret[++ret_index];
			set_index = charset.find(original);
		}
	}

	return ret;
}


int init(const std::string& port) {
	std::printf("initializing server\n");

	i_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (i_result != 0) {
		std::printf("WSAStartup failed: %d\n", i_result);
		return 1;
	}


	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;


	i_result = getaddrinfo(NULL, port.c_str(), &hints, &result);
	if (i_result != 0) {
		std::printf("getaddrinfo failed: %d\n", i_result);
		WSACleanup();
		return 1;
	}

	server = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (server == INVALID_SOCKET) {
		std::printf("socket failed: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}


	i_result = bind(server, result->ai_addr, (int)result->ai_addrlen);
	if (i_result == SOCKET_ERROR) {
		std::printf("bind failed: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(server);
		WSACleanup();
		return 1;
	}


	freeaddrinfo(result);


	i_result = listen(server, SOMAXCONN);
	if (i_result == SOCKET_ERROR) {
		std::printf("listen failed: %d\n", WSAGetLastError());
		closesocket(server);
		WSACleanup();
		return 1;
	}

	std::printf("server bound\n");
	return 0;
}


int main(int argc, char* argv[]) {
	srand((unsigned)time(0));

	std::string port = "12345";
	
	if (argc > 1) {
		dnloadpath = argv[1];
		if (dnloadpath[dnloadpath.size() - 1] != '/' && dnloadpath[dnloadpath.size() - 1] != '\\') {
			dnloadpath += (dnloadpath.find('/') != -1) ? '/' : '\\';
		}
		_mkdir(dnloadpath.c_str());
		if (argc > 2) {
			port = argv[2];
		}
	}
	else {
		std::cout << "ERROR: No download path provided.\n\nProper execution format:\n*.exe [download path] [port]\n\tnote: surround the download path with quotes if it contains spaces\n\n";
		return 1;
	}

	std::string greet = "---File Transfer Server---", cinfo = "port: " + port;

	int largest = (int)max(greet.size(), dnloadpath.size());

	std::cout << "\n" << std::string((largest - greet.size()) / 2, '-') << greet << std::string(largest - greet.size() - (largest - greet.size()) / 2, '-') << "\n";
	std::cout << std::string((largest - dnloadpath.size()) / 2, ' ') << dnloadpath << "\n";
	std::cout << std::string((largest - cinfo.size()) / 2, ' ') << cinfo << "\n\n";



	std::ostringstream oss;
	for (auto & p : std::experimental::filesystem::directory_iterator(dnloadpath)) {
		if (std::experimental::filesystem::is_directory(p.status())) continue;
		oss << p;
		files.insert(std::pair<std::string, std::string>(genkey(CURRENT_SET, CURRENT_SET.size()), oss.str()));
		oss.str("");
	}


	for (std::size_t i = 0; i < MAX_CONNECTIONS; ++i) {
		connections[i] = INVALID_SOCKET;
	}

	init(port);


	while (1) {
		client = INVALID_SOCKET;

		client = accept(server, NULL, NULL);

		if (client == INVALID_SOCKET) {
			std::printf("accept failed: %d\nre-initializing server socket\n", WSAGetLastError());
			closesocket(server);
			WSACleanup();
			i_result = init(port);
			if (i_result != 0) return i_result;
			continue;
		}

		std::printf("client connected\n");


		for (count = 0; count < MAX_CONNECTIONS; ++count) {
			if (connections[count] == INVALID_SOCKET) {
				DWORD threadID;
				CreateThread(0, 0, run, (void*)client, 0, &threadID);
				std::cout << "new client thread started.\n\n";
				break;
			}
		}


		if (count == MAX_CONNECTIONS) {
			std::cout << "max number of connections reached. closing client socket.\n\n";

			char m[] = "maximum number of connections reached.\n";
			i_result = send(client, m, (int)strlen(m), 0);
			if (i_result == SOCKET_ERROR) {
				std::printf("send failed: %d\n", WSAGetLastError());
			}
			else {
				std::printf("sent %d bytes\n", i_result);
			}

			closesocket(client);
		}
	}


	return 0;
}





DWORD WINAPI run(void* ptr) {
	SOCKET sock = (SOCKET)ptr;
	connections[count] = sock;
	short id = count;

	Communicator com(sock, id, dnloadpath);
	COM_CODE result;

	int num = -1;


	std::string rec = "", cm = "", info = "";
	int i_result;
	bool exists = true;
	while (1) {
		i_result = 0;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&i_result, sizeof(int));

		rec.clear(); rec.resize(DEFAULT_BUFLEN);
		i_result = recv(sock, &rec[0], (int)rec.size(), 0);

		if (i_result > 0) {
			std::printf("%d/ received %d bytes: %s\n", id, i_result, rec.c_str());
		}
		else if (i_result == 0) {
			std::printf("%d/ connection closed\n", id);
			break;
		}
		else {
			std::printf("%d/ recv failed: %d\n", id, WSAGetLastError());
			break;
		}

		rec.resize(i_result);

		cm = rec.substr(0, rec.find(' '));

		if (cm == "fsend") {
			result = com.getFile(rec.substr(cm.size() + 1), info, ++num, 30000);
			if (result == COM_SUCCESS) {
				files.insert(std::pair<std::string, std::string>(genkey(CURRENT_SET, CURRENT_SET.size()), info));
				std::printf("%d/ file download successful.\n\n", id);
				continue;
			}
			else if (result == BAD_REPLY) {
				std::printf("%d/ ERROR %d: unexpected reply.\n\n", id, result);
				continue;
			}
			else if (result == MISSING_DATA) {
				std::printf("%d/ ERROR %d: missing file data, file was removed.\n", id, result);
			}
			else if (result == EXTRA_DATA) {
				std::printf("%d/ ERROR %d: received extra file data, file was removed.\n", id, result);
			}
		}
		else if (cm.find("fget") == 0) {
			info = rec.substr(cm.size() + 1);
			exists = true;

			if (cm == "fget-id") {
				if (files.find(info) == files.end()) {
					std::printf("%d/ requested file does not exist.\n", id);
					exists = false;
				}

				result = com.sendFile((exists) ? files[info] : "", exists, 30000);
			}
			else if (cm == "fget") {
				std::map<std::string, std::string>::const_iterator iter;
				for (iter = files.begin(); iter != files.end(); ++iter) {
					if (iter->second.find(info) != -1) break;
				}

				if (iter == files.end()) {
					std::printf("%d/ requested file does not exist.\n", id);
					exists = false;
				}

				result = com.sendFile((exists) ? iter->second : "", exists, 30000);
			}
			else {
				continue;
			}


			if (result == COM_SUCCESS && exists) {
				std::printf("%d/ file upload successful.\n\n", id);
				continue;
			}
			else if (result == BAD_REPLY) {
				std::printf("%d/ ERROR %d: unexpected reply.\n\n", id, result);
				continue;
			}
			else if (result == MISSING_DATA) {
				std::printf("%d/ ERROR %d: missing file data.\n", id, result);
			}
			else if (result == EXTRA_DATA) {
				std::printf("%d/ ERROR %d: extra file data.\n", id, result);
			}
		}
		else if (cm == "lget") {
			result = com.sendList(files, 30000);
			if (result == COM_SUCCESS) {
				std::printf("%d/ file list successfully sent.\n\n", id);
				continue;
			}
		}
		else if (cm.find("dget") == 0) {
			info = rec.substr(cm.size() + 1);

			if (cm == "dget-id") {
				if (files.find(info) == files.end()) {
					std::printf("%d/ requested file does not exist.\n", id);
					result = com.sendData("", "", 30000);
				}
				else {
					result = com.sendData(files[info], info, 30000);
				}
			}
			else if (cm == "dget") {
				std::map<std::string, std::string>::const_iterator iter;
				for (iter = files.begin(); iter != files.end(); ++iter) {
					if (iter->second.find(info) != -1) break;
				}

				if (iter == files.end()) {
					std::printf("%d/ requested file does not exist.\n", id);
					result = com.sendData("", "", 30000);
				}
				else {
					result = com.sendData(iter->second, iter->first, 30000);
				}
			}
			else {
				continue;
			}

			if (result == COM_SUCCESS) {
				std::printf("%d/ file data successfully sent.\n\n", id);
				continue;
			}
		}


		if (result == NO_CONNECTION) {
			std::printf("%d/ ERROR %d: no connection.\n", id, result);
			break;
		}
		else if (result == BAD_REPLY) {
			std::printf("%d/ ERROR %d: unexpected reply.\n", id, result);
			break;
		}
		else if (result == RECV_FAILED) {
			std::printf("%d/ ERROR %d: receive failed.\n", id, result);
		}

		std::printf("\n");
	}

	close(sock, id);

	return 0;
}


void close(SOCKET& client, short id) {
	lock.lock();

	closesocket(client);
	connections[id] = INVALID_SOCKET;
	std::cout << id << "/ socket closed.\n";

	lock.unlock();
}