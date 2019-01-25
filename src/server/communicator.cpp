#include "communicator.h"
#include "file.h"

#include <iostream>
#include <stdio.h>


Communicator::Communicator(SOCKET& sock, short id, const std::string& dnloadpath) :
	sock(sock), id(id), dnloadpath(dnloadpath) {
}



COM_CODE Communicator::getFile(const std::string& filename, std::string& file, int num, int timeout) {
	int dataamnt = 0, bytesrecvd = 0;

	//Set socket timeout in milliseconds (0 indicates blocking)
	if (timeout < 0) timeout = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));



	//Send ready string
	info = "00000000";
	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << id << "/ send failed: " << i_result << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}

	std::printf("%d/ sent %d bytes: %s\n", id, i_result, info.c_str());



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

	std::printf("%d/ received %d bytes: %d\n", id, i_result, dataamnt);


	if (dataamnt <= 0) {
		std::printf("%d/ the file is empty, stopping download.\n", id);
		info = "-1";
		i_result = send(sock, &info[0], (int)info.size(), 0);
		if (i_result == SOCKET_ERROR) {
			std::cerr << id << "/ send failed: " << WSAGetLastError() << "\n";
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
		std::cerr << id << "/ send failed: " << WSAGetLastError() << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}

	std::printf("%d/ sent %d bytes: %s\n", id, i_result, info.c_str());



	//Wait for file data
	file = dnloadpath + std::to_string(id) + "_" + std::to_string(getTime() + num) + "-" + filename;
	File f(file);
	std::vector<char> data;
	std::printf("%d/ downloading to: %s\n", id, (f.path() + f.name()).c_str());
	while (bytesrecvd < dataamnt) {
		data.clear(); data.resize(DEFAULT_BUFLEN);
		i_result = recv(sock, &data[0], DEFAULT_BUFLEN, 0);

		if (i_result == 0) {
			return NO_CONNECTION;
		}
		else if (i_result < 0) {
			break;
		}

		std::printf("%d/ received %d bytes.\n", id, i_result);

		data.resize(i_result);
		f.write(data, 1);

		bytesrecvd += i_result;
	}


	//Send ack
	info = std::to_string(bytesrecvd);
	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << id << "/ send failed: " << WSAGetLastError() << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}

	std::printf("%d/ sent %d bytes.\n", id, i_result);



	if (bytesrecvd != dataamnt) {
		std::remove((dnloadpath + filename).c_str());

		if (bytesrecvd < dataamnt) {
			return MISSING_DATA;
		}
		else {
			return EXTRA_DATA;
		}
	}

	return COM_SUCCESS;
}


COM_CODE Communicator::sendList(const std::map<std::string, std::string>& files, int timeout) {
	COM_CODE ret = COM_SUCCESS;

	//Set socket timeout in milliseconds (0 indicates blocking)
	if (timeout < 0) timeout = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));



	//Send data for each file
	info = genFileList(files);
	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << id << "/ send failed: " << i_result << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}
	std::printf("%d/ sent %d bytes\n", id, i_result);

	compare = (int)info.size();


	//Wait for ack and check if correct
	info.clear(); info.resize(DEFAULT_BUFLEN);
	i_result = recv(sock, &info[0], DEFAULT_BUFLEN, 0);
	if (i_result == 0) {
		return NO_CONNECTION;
	}
	else if (i_result < 0) {
		return RECV_FAILED;
	}
	std::printf("%d/ received %d bytes: %s\n", id, i_result, info.c_str());

	info.resize(i_result);

	if (std::stoi(info) != compare) {
		ret = BAD_REPLY;
	}



	//Send finish string
	i_result = send(sock, &FINISH_MESSAGE[0], (int)FINISH_MESSAGE.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << id << "/ send failed: " << i_result << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}
	std::printf("%d/ sent %d bytes: %s\n", id, i_result, FINISH_MESSAGE.c_str());


	return ret;
}

COM_CODE Communicator::sendData(const std::string& filepath, const std::string& fileid, int timeout) {
	//Set socket timeout in milliseconds (0 indicates blocking)
	if (timeout < 0) timeout = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));



	//Send data for file
	File file(filepath);
	info = "name-\t" + file.name() + "\n  id-\t" + fileid + "\nsize-\t" + std::to_string(file.size()) + " bytes\n";

	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << id << "/ send failed: " << i_result << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}
	std::printf("%d/ sent %d bytes\n", id, i_result);

	compare = (int)info.size();


	//Wait for ack and check if correct
	info.clear(); info.resize(DEFAULT_BUFLEN);
	i_result = recv(sock, &info[0], DEFAULT_BUFLEN, 0);
	if (i_result == 0) {
		return NO_CONNECTION;
	}
	else if (i_result < 0) {
		return RECV_FAILED;
	}
	std::printf("%d/ received %d bytes: %s\n", id, i_result, info.c_str());

	info.resize(i_result);

	if (std::stoi(info) != compare) {
		return BAD_REPLY;
	}



	//Send finish string
	i_result = send(sock, &FINISH_MESSAGE[0], (int)FINISH_MESSAGE.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << id << "/ send failed: " << i_result << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}
	std::printf("%d/ sent %d bytes: %s\n", id, i_result, FINISH_MESSAGE.c_str());


	return COM_SUCCESS;
}

COM_CODE Communicator::sendFile(const std::string& filepath, bool exists, int timeout) {
	//Send ready string
	info = (exists) ? READY_MESSAGE : FINISH_MESSAGE;
	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << id << "/ send failed: " << i_result << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}

	std::printf("%d/ sent %d bytes: %s\n", id, i_result, info.c_str());
	if (!exists) return COM_SUCCESS;

	compare = (int)info.size();


	//Wait for ack and check if correct
	info.clear(); info.resize(DEFAULT_BUFLEN);
	i_result = recv(sock, &info[0], DEFAULT_BUFLEN, 0);
	if (i_result == 0) {
		return NO_CONNECTION;
	}
	else if (i_result < 0) {
		return RECV_FAILED;
	}
	std::printf("%d/ received %d bytes: %s\n", id, i_result, info.c_str());

	info.resize(i_result);

	if (std::stoi(info) != compare) {
		return BAD_REPLY;
	}


	//Send file name
	File f(filepath);

	info = f.name().substr(f.name().find_first_of('-') + 1);
	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << id << "/ send failed: " << i_result << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}
	std::printf("%d/ sent %d bytes: %s\n", id, i_result, info.c_str());

	int compare = (int)info.size();


	//Wait for ack and check if correct
	info.clear(); info.resize(DEFAULT_BUFLEN);
	i_result = recv(sock, &info[0], DEFAULT_BUFLEN, 0);
	if (i_result == 0) {
		return NO_CONNECTION;
	}
	else if (i_result < 0) {
		return RECV_FAILED;
	}
	info.resize(i_result);
	std::printf("%d/ received %d bytes: %s\n", id, i_result, info.c_str());

	if (std::stoi(info) != compare) {
		return BAD_REPLY;
	}

	
	//Send file size
	info = std::to_string(f.size());
	i_result = send(sock, &info[0], (int)info.size(), 0);
	if (i_result == SOCKET_ERROR) {
		std::cerr << id << "/ send failed: " << i_result << "\n";
		closesocket(sock);
		WSACleanup();
		return NO_CONNECTION;
	}
	std::printf("%d/ sent %d bytes: %s\n", id, i_result, info.c_str());

	compare = (int)info.size();


	//Wait for ack and check if correct
	info.clear(); info.resize(DEFAULT_BUFLEN);
	i_result = recv(sock, &info[0], DEFAULT_BUFLEN, 0);
	if (i_result == 0) {
		return NO_CONNECTION;
	}
	else if (i_result < 0) {
		return RECV_FAILED;
	}
	info.resize(i_result);
	std::printf("%d/ received %d bytes: %s\n", id, i_result, info.c_str());

	if (std::stoi(info) != compare) {
		return BAD_REPLY;
	}



	//Send file data
	int bytes = (int)f.size();
	int progress = 0, buflen = DEFAULT_BUFLEN;

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
		std::printf("%d/ sent %d/%d bytes\n", id, progress, bytes);
	} while (progress < bytes);

	compare = (int)f.size();


	//Wait for ack and check if correct
	info.clear(); info.resize(DEFAULT_BUFLEN);
	i_result = recv(sock, &info[0], DEFAULT_BUFLEN, 0);
	if (i_result == 0) {
		return NO_CONNECTION;
	}
	else if (i_result < 0) {
		return RECV_FAILED;
	}
	info.resize(i_result);
	std::printf("%d/ received %d bytes: %s\n", id, i_result, info.c_str());


	info.resize(i_result);
	i_result = std::stoi(info);
	if (i_result != compare) {
		if (i_result < compare) {
			return MISSING_DATA;
		}
		else {
			return EXTRA_DATA;
		}
	}


	return COM_SUCCESS;
}



int getTime() {
	return static_cast<int>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
}




std::string Communicator::genFileList(const std::map<std::string, std::string>& files) {
	const static char div = '_';
	const static std::size_t idlen = 10, namelen = 24, filelen = 32;
	std::size_t numlen = std::to_string(files.size()).size() + 2;
	const static std::string divider = "|" + std::string(numlen, div) + "|" + std::string(idlen, div) + "|" + std::string(namelen, div) + "|" + std::string(filelen, div) + "|\n";
	std::string list = "| #" + std::string(numlen - 2, ' ') + "|    ID    |          NAME          |              FILE              |\n" + divider;

	std::string n_holder, f_holder, fname, nopath;
	std::size_t ni, fi, count = 0;
	std::string hold;
	for (std::map<std::string, std::string>::const_iterator iter = files.begin(); iter != files.end(); ++iter) {
		ni = 0;
		fi = 0;
		nopath = iter->second.substr(((iter->second.find("\\") != -1) ? iter->second.find_last_of("\\") : iter->second.find_last_of("/")) + 1);
		fname = nopath.substr(nopath.find_first_of('-') + 1);
		do {
			try {
				n_holder = fname.substr(ni, (ni + namelen - 2 > fname.size()) ? std::string::npos : namelen - 2);
				f_holder = nopath.substr(fi, (fi + filelen - 2 > nopath.size()) ? std::string::npos : filelen - 2);

				hold = (ni == 0) ? std::to_string(++count) : " ";
				list.append("| " + hold + std::string(numlen - 2 - hold.size(), ' ') + " | "
					+ ((ni == 0) ? iter->first : std::string(idlen - 2, ' ')) + " | "
					+ n_holder + std::string(namelen - 2 - n_holder.size(), ' ') + " | "
					+ f_holder + std::string(filelen - 2 - f_holder.size(), ' ') + " | \n");

				ni += namelen - 2;
				fi += filelen - 2;
			}
			catch (std::out_of_range e) { break; }
		} while (ni < fname.size() || fi < nopath.size());
		list.append(divider);
	}

	return list;
}