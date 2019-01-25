#include "parser.h"
#include "communicator.h"

#include <iostream>
#include <string>
#include <math.h>


int main(int argc, char* argv[]) {
	std::string dnloadpath = "", ip = "127.0.0.1", port = "12345";
	if (argc > 1) {
		dnloadpath = argv[1];
		if (dnloadpath[dnloadpath.size() - 1] != '/' || dnloadpath[dnloadpath.size() - 1] != '\\') {
			dnloadpath += (dnloadpath.find('/') != -1) ? '/' : '\\';
		}
		if (argc > 2) {
			ip = argv[2];
			if (argc > 3) {
				port = argv[3];
			}
		}
	}
	else {
		std::cout << "ERROR: No download path provided.\n\nProper execution format:\n*.exe [download path] [server ip] [server port]\n\tnote: surround the download path with quotes if it contains spaces\n\n";
		return 1;
	}
	
	std::string greet = "---File Transfer Client---", cinfo = ip + "::" + port;

	int largest = (int)max(greet.size(), dnloadpath.size());

	std::cout << "\n" << std::string((largest - greet.size()) / 2, '-') << greet << std::string(largest - greet.size() - (largest - greet.size()) / 2, '-') << "\n";
	std::cout << std::string((largest - dnloadpath.size()) / 2, ' ') << dnloadpath << "\n";
	std::cout << std::string((largest - cinfo.size()) / 2, ' ') << cinfo << "\n\n";

	csocket sock(ip, port);

	std::cout << "> ";
	std::string line;
	PARSE_CODE code;
	while (getline(std::cin, line)) {
		code = input(line, sock, dnloadpath);
		if (code == TERMINATE) {
			break;
		}
		std::cout << "> ";
	}

	return 0;
}