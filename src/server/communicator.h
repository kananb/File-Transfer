#ifndef GUARD_COMMUNICATOR_H
#define GUARD_COMMUNICATOR_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include <string>
#include <map>
#include <chrono>


enum COM_CODE {
	COM_SUCCESS, NO_CONNECTION, BAD_REPLY, RECV_FAILED, MISSING_DATA, EXTRA_DATA
};


const int DEFAULT_BUFLEN = 134217728;
const std::string READY_MESSAGE = "00000000";
const std::string FINISH_MESSAGE = "11111111";

class Communicator {
public:
	Communicator(SOCKET&, short, const std::string&);


	COM_CODE getFile(const std::string&, std::string&, int, int = 0);

	COM_CODE sendList(const std::map<std::string, std::string>&, int = 0);
	COM_CODE sendData(const std::string&, const std::string&, int = 0);
	COM_CODE sendFile(const std::string&, bool = 1, int = 0);

private:
	std::string genFileList(const std::map<std::string, std::string>&);


	SOCKET sock;
	short id;
	std::string dnloadpath;

	std::string info;
	int i_result, compare;
};


int getTime();

#endif //GUARD_COMMUNICATOR_H