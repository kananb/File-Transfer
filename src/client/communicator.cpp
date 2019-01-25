#include "communicator.h"


//variables
std::string data;
std::string info = "";
int i_result = 0;

long dataamnt = 0;
std::string filename = "";
long bytesrecvd = 0;



COM_CODE sendFile(const std::string& path, const std::string& name, bool showOut, int buflen, csocket& csock) {
	SOCKET sock = csock.getSocket();

	//Send file indicator to server
	info = "fsend " + name;
	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << "send failed: " << i_result << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}
	if (showOut) std::printf("sent %d bytes: %s\n", i_result, info.c_str());


	//Wait for ready string
	info.clear(); info.resize(DEFAULT_BUFLEN);
	i_result = recv(sock, &info[0], DEFAULT_BUFLEN, 0);
	if (i_result == 0) {
		std::cout << "no connection\n";
		return NO_CONNECTION;
	}
	else if (i_result < 0) {
		std::cout << "recv failed\n";
		return RECV_FAILED;
	}
	info.resize(i_result);
	if (showOut) std::printf("received %d bytes: %s\n", i_result, info.c_str());

	if (info != "00000000") {
		std::cout << "server is not able to receive the file.\n";
		return BAD_REPLY;
	}



	//Send file size
	File f = path;

	info = std::to_string(f.size());
	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << "send failed: " << i_result << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}
	if (showOut) std::printf("sent %d bytes: %s\n", i_result, info.c_str());

	int compare = (int)info.size();


	//Wait for ack and check if correct
	info.clear(); info.resize(DEFAULT_BUFLEN);
	i_result = recv(sock, &info[0], DEFAULT_BUFLEN, 0);
	if (i_result == 0) {
		std::printf("no connection.\n");
		return NO_CONNECTION;
	}
	else if (i_result < 0) {
		std::printf("recv failed.\n");
		return RECV_FAILED;
	}
	info.resize(i_result);
	if (showOut) std::printf("received %d bytes: %s\n", i_result, info.c_str());

	if (std::stoi(info) != compare) {
		std::printf("unexpected data: %s\n", info.c_str());
		if (f.size() <= 0) {
			std::printf("the selected file is empty, stopping upload.\n");
		}
		return BAD_REPLY;
	}



	//Send file data
	int bytes = (int)f.size();
	int progress = 0;

	if (buflen <= 0 || buflen > bytes) {
		buflen = bytes;
	}

	do {
		if (progress + buflen > bytes) buflen = bytes - progress;
		i_result = send(sock, &f.contents()[0] + progress, buflen, 0);

		if (i_result == SOCKET_ERROR) {
			std::cerr << "send failed: " << WSAGetLastError() << "\n";
			closesocket(sock);
			WSACleanup();
			return NO_CONNECTION;
		}

		progress += i_result;
		if (showOut) std::printf("sent %d/%d bytes\n", progress, bytes);
	} while (progress < bytes);

	compare = (int)f.size();


	//Wait for ack and check if correct
	info.clear(); info.resize(DEFAULT_BUFLEN);
	i_result = recv(sock, &info[0], DEFAULT_BUFLEN, 0);
	if (i_result == 0) {
		std::printf("no connection.\n");
		return NO_CONNECTION;
	}
	else if (i_result < 0) {
		std::printf("recv failed.\n");
		return RECV_FAILED;
	}
	if (showOut) std::printf("received %d bytes\n", i_result);


	info.resize(i_result);
	i_result = std::stoi(info);
	if (i_result != compare) {
		std::printf("file upload failed: ");
		if (i_result < compare) {
			std::printf("missing data.\n");
			return MISSING_DATA;
		}
		else {
			std::printf("extra data.\n");
			return EXTRA_DATA;
		}
	}


	return COM_SUCCESS;
}



COM_CODE getAndPrint(const std::string& message, bool showOut, int buflen, csocket& csock) {
	const static std::string FINISH_MESSAGE = "11111111";
	SOCKET sock = csock.getSocket();
	int total_bytes = 0;

	//Send list/data indicator to server
	i_result = send(sock, &message[0], (int)message.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << "send failed: " << i_result << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}
	if (showOut) std::printf("sent %d bytes: %s\n", i_result, message.c_str());



	//Receive information and wait for finish message
	while (1) {
		info.clear(); info.resize(DEFAULT_BUFLEN);
		i_result = recv(sock, &info[0], DEFAULT_BUFLEN, 0);
		if (i_result == 0) {
			std::cout << "no connection\n";
			return NO_CONNECTION;
		}
		else if (i_result < 0) {
			std::cout << "recv failed\n";
			return RECV_FAILED;
		}
		info.resize(i_result);
		total_bytes += i_result;

		
		if (info == FINISH_MESSAGE) break;
		else {
			std::printf("%s", info.c_str());
		}

		
		//Send ack
		info = std::to_string(i_result);
		i_result = send(sock, &info[0], (int)info.size(), 0);
		if (i_result == SOCKET_ERROR) {
			std::cerr << "send failed: " << WSAGetLastError() << "\n";
			closesocket(sock);
			WSACleanup();
			return NO_CONNECTION;
		}
		if (showOut) std::printf("sent %d bytes: %s\n", i_result, info.c_str());
	}

	if (showOut) std::printf("received %d bytes\n", i_result);

	
	return COM_SUCCESS;
}

COM_CODE getList(csocket& sock) {
	return getAndPrint("lget", false, -1, sock);
}

COM_CODE getData(const std::string& name, bool showOut, bool isId, int buflen, csocket& sock) {
	return getAndPrint("dget" + (((isId) ? "-id " : " ") + name), showOut, buflen, sock);
}

COM_CODE getFile(const std::string& file, bool showOut, bool isId, int buflen, csocket& csock, const std::string& dnloadpath, const std::string& fnoption) {
	SOCKET sock = csock.getSocket();
	std::string name = file;

	//Set socket timeout in milliseconds (0 indicates blocking)
	int timeout = 30000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));


	//Send get file indicator to server
	info = "fget" + (((isId) ? "-id " : " ") + file);
	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << "send failed: " << WSAGetLastError() << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}


	//Wait for ready message
	info.clear(); info.resize(DEFAULT_BUFLEN);
	i_result = recv(sock, &info[0], DEFAULT_BUFLEN, 0);
	if (i_result == 0) {
		std::cout << "no connection\n";
		return NO_CONNECTION;
	}
	else if (i_result < 0) {
		std::cout << "recv failed\n";
		return RECV_FAILED;
	}
	info.resize(i_result);
	if (showOut) std::printf("received %d bytes: %s\n", i_result, info.c_str());

	if (info != "00000000") {
		std::cout << "server is not able to send the file or it doesn't exist.\n";
		return BAD_REPLY;
	}


	//Send ack
	info = std::to_string(i_result);
	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << "send failed: " << WSAGetLastError() << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}
	if (showOut) std::printf("sent %d bytes: %s\n", i_result, info.c_str());


	//Wait for file name
	info.clear(); info.resize(DEFAULT_BUFLEN);
	i_result = recv(sock, &info[0], DEFAULT_BUFLEN, 0);
	if (i_result == 0) {
		return NO_CONNECTION;
	}
	else if (i_result < 0) {
		return RECV_FAILED;
	}
	info.resize(i_result);
	name = info;

	if (showOut) std::printf("received %d bytes: %s\n", i_result, info.c_str());

	if (fnoption != "") filename = fnoption;
	else filename = name;


	//Send ack
	info = std::to_string(i_result);
	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << "send failed: " << WSAGetLastError() << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}

	if (showOut) std::printf("sent %d bytes: %s\n", i_result, info.c_str());


	//Wait for file size
	info.clear(); info.resize(DEFAULT_BUFLEN);
	i_result = recv(sock, &info[0], DEFAULT_BUFLEN, 0);
	if (i_result == 0) {
		return NO_CONNECTION;
	}
	else if (i_result < 0) {
		return RECV_FAILED;
	}
	info.resize(i_result);
	dataamnt = std::stoi(info);

	if (showOut) std::printf("received %d bytes: %d\n", i_result, dataamnt);


	if (dataamnt <= 0) {
		std::printf("the file is empty, stopping download.\n");
		info = "-1";
		i_result = send(sock, &info[0], (int)info.size(), 0);
		if (i_result == SOCKET_ERROR) {
			std::cerr << "send failed: " << WSAGetLastError() << "\n";
			closesocket(sock);
			WSACleanup();
			return NO_CONNECTION;
		}

		return BAD_REPLY;
	}


	//Send ack
	info = std::to_string(i_result);
	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << "send failed: " << WSAGetLastError() << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}

	if (showOut) std::printf("sent %d bytes: %s\n", i_result, info.c_str());



	//Wait for file data
	File f(dnloadpath + filename);
	std::vector<char> vdata;

	if (buflen < 0) buflen = DEFAULT_BUFLEN;

	bytesrecvd = 0;
	while (bytesrecvd < dataamnt) {
		vdata.clear(); vdata.resize(buflen);
		i_result = recv(sock, &vdata[0], buflen, 0);

		if (i_result == 0) {
			return NO_CONNECTION;
		}
		else if (i_result < 0) {
			break;
		}

		if (showOut) std::printf("received %d bytes.\n", i_result);

		vdata.resize(i_result);
		f.write(vdata, 1);

		bytesrecvd += i_result;
	}


	//Send ack
	info = std::to_string(bytesrecvd);
	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << "send failed: " << WSAGetLastError() << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}

	if (showOut) std::printf("sent %d bytes.\n", i_result);



	if (bytesrecvd != dataamnt) {
		std::printf("file download failed: ");
		if (bytesrecvd < dataamnt) {
			std::printf("missing data.\n");
			return MISSING_DATA;
		}
		else {
			std::printf("extra data.\n");
			return EXTRA_DATA;
		}
	}

	std::printf("file download successful.\n");
	return COM_SUCCESS;
}