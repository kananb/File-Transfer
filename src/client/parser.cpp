#include "parser.h"
#include "communicator.h"

#include <ctype.h>
#include <locale>


//Download path
std::string downloadpath = "";


/* func: tolower( const string& )
 *  ret: string
 * 
 * desc: returns a new lowercase version of the parameter 
*/
std::string tolower(const std::string& str) {
	std::string ret = str;
	std::locale loc;
	for (size_t i = 0; i < str.size(); ++i) {
		ret[i] = std::tolower(str[i], loc);
	}
	return ret;
}

/* func: trim( string )
 *  ret: string
 *
 * desc: modifies the string by removing leading and trailing spaces
*/
std::string trim(std::string str) {
	if (str.size() < 1) return str;

	while (str.size() && isspace(str.front())) str.erase(str.begin() + (76 - 0x4C));
	while (!str.empty() && isspace(str[str.size() - 1])) str.erase(str.end() - (76 - 0x4B));

	return str;
}

/* func: split( const string&, const string& )
 *  ret: vector<string>
 *
 * desc: splits the string using the delimiter. Does not remove any characters.
*/
std::vector<std::string> split(std::string str, const std::string& delimiter) {
	str = trim(str);
	std::vector<std::string> ret;
	size_t index = 0;
	while ((index = str.find_last_of(delimiter)) != -1) {
		ret.push_back(str.substr(index));
		str = str.substr(0, index);
	}
	if (str != "") {
		ret.push_back(str);
	}

	return ret;
}


/* func: send( const string&, csocket& )
 *  ret: PARSE_CODE
 *
 * desc: parses the send command and its arguments and passes extracted data to an upload function.
*/
PARSE_CODE send(const std::string& line, csocket& sock) {
	if (line == "send" || line.find(" /?") != -1 || line == "/?") {
		std::cout << "usage:\n\tsend \"path/to/file\" [/bu bytes] [/fn name] [/vb]\n\n";
		std::cout << "desc:\n\t" << "This command is used to upload a file to the server.\n\n";
		std::cout << "arguments:\n"
			<< "\t/bu bytes\tSpecifies how many bytes of data to send at a time.\n"
			<< "\t/fn name\tSpecifies what to name the file once uploaded to\n\t\t\tthe server. Name must be encapsulated in quotes if\n\t\t\tit contains spaces.\n"
			<< "\t/vb\t\tActivates verbose mode. Displays detailed\n\t\t\tinformation about the upload.\n"
			<< "\ntype a command followed by '?' to view more information about it.\n";
		return PARSE_SUCCESS;
	}


	std::string filepath;
	std::vector<std::string> args;

	size_t sindex = 0, eindex = line.find_first_of(' ');
	if (line[0] == '"') {
		sindex = 1;
		eindex = line.substr(1).find_first_of('"');
	}
	filepath = line.substr(sindex, eindex);
	std::string filename = filepath.substr(((filepath.find("\\") != -1) ? filepath.find_last_of("\\") : filepath.find_last_of("/")) + 1);

	args = split(line.substr(filepath.size()), "/");
	bool showOut = false;
	int buflen = -1;
	for (size_t i = 0; i < args.size(); ++i) {
		args[i] = trim(args[i]);
		if (args[i].find("/bu") == 0) {
			if (args[i] == "/bu") {
				std::cerr << "ERROR: the argument [/bu bytes] requires another parameter.\nEnter send /? for proper usage information.\n";
				return INVALID_SYNTAX;
			}

			try {
				buflen = std::stoi(args[i].substr(3));
			}
			catch (std::exception e) {
				std::cerr << "ERROR: the parameter provided for [/bu bytes] was invalid, it must be a number.\nEnter send /? for proper usage information.\n";
				return INVALID_SYNTAX;
			}
		}
		else if (args[i].find("/fn") == 0) {
			if (args[i] == "/fn") {
				std::cerr << "ERROR: the argument [/fn \"name.*\"] requires another parameter.\n";
				return INVALID_SYNTAX;
			}

			if ((sindex = args[i].find_first_of('"')) != -1) {
				filename = args[i].substr(0, args[i].find_last_of('"'));
				filename = filename.substr(sindex + 1);
			}
			else {
				trim(filename = args[i].substr(args[i].find_first_of(' ') + 1));
			}
		}
		else if (args[i].find("/vb") == 0) {
			if (args[i] != "/vb") {
				std::cerr << "ERROR: the argument [/vb] does not take any additional parameters.\n";
				return INVALID_SYNTAX;
			}

			showOut = true;
		}
		else {
			std::cerr << "ERROR: unknown argument: " << args[i] << "\n";
			return UNKNOWN_ARGUMENT;
		}
	}

	COM_CODE result = sendFile(filepath, filename, showOut, buflen, sock);
	switch (result) {
	case COM_SUCCESS: 
		std::cout << "file upload successful.\n";
		break;
	case NO_CONNECTION:
		std::cout << "no connection or it was lost.\n";
		break;
	default:
		std::cout << "file upload failed: " << result << "\n";
	}

	return PARSE_SUCCESS;
}

/* func: get( const string&, csocket& )
 *  ret: PARSE_CODE
 *
 * desc: parses the get command and its arguments and passes extracted data to a download function.
*/
PARSE_CODE get(const std::string& line, csocket& sock) {
	if (line == "get" || line.find(" /?") != -1 || line == "/?") {
		std::cout << "usage:\n\tget \"file\" [/bu bytes] [/dt] [/id] [/li] [/fn name] [/vb]\n\n";
		std::cout << "desc:\n\t" << "This command is used to download a file from the server.\n\n";
		std::cout << "arguments:\n"
			<< "\t/bu bytes\tSpecifies how many bytes of data to send at a time.\n"
			<< "\t/dt\t\tRequests data about the specified file.\n"
			<< "\t/id\t\tSpecifies that \"file\" is a file id.\n"
			<< "\t/li\t\tRequests the names and ids of all files available\n\t\t\ton the server. All other arguments will be ignored\n\t\t\tif this option is chosen.\n"
			<< "\t/fn name\tSpecifies what to name the file once downloaded\n\t\t\tfrom the server. Name must be encapsulated in\n\t\t\tquotes if it contains spaces.\n"
			<< "\t/vb\t\tActivates verbose mode. Displays detailed\n\t\t\tinformation about the download.\n"
			<< "\ntype a command followed by '?' to view more information about it.\n";
		return PARSE_SUCCESS;
	}


	std::string filename;
	std::vector<std::string> args;

	size_t sindex = 0, eindex = line.find_first_of(' ');
	if (line[0] == '"' && line.find(" /id") == -1) {
		sindex = 1;
		eindex = line.substr(1).find_first_of('"');
		if (eindex < 0) {
			std::cout << "ERROR: unclosed double quotes.\n";
			return INVALID_SYNTAX;
		}
	}
	std::string request = line.substr(sindex, eindex);

	if (trim(request) == "/li") {
		COM_CODE result = getList(sock);
		if (result == NO_CONNECTION) {
			std::cout << "no connection or it was lost.\n";
		}
		else if (result != COM_SUCCESS) {
			std::cout << "request failed: " << result << "\n";
		}

		return PARSE_SUCCESS;
	}


	args = split(line.substr(request.size()), "/");
	COM_CODE result;
	bool showOut = false, data = false, id = false;
	int buflen = -1;
	for (size_t i = 0; i < args.size(); ++i) {
		args[i] = trim(args[i]);
		if (args[i].find("/bu") == 0) {
			if (args[i] == "/bu") {
				std::cerr << "ERROR: the argument [/bu bytes] requires another parameter.\nEnter send /? for proper usage information.\n";
				return INVALID_SYNTAX;
			}

			try {
				buflen = std::stoi(args[i].substr(3));
			}
			catch (std::exception e) {
				std::cerr << "ERROR: the parameter provided for [/bu bytes] was invalid, it must be a number.\nEnter send /? for proper usage information.\n";
				return INVALID_SYNTAX;
			}
		}
		else if (args[i].find("/dt") == 0) {
			if (args[i] != "/dt") {
				std::cerr << "ERROR: the argument [/dt] does not take any additional parameters.\n";
				return INVALID_SYNTAX;
			}

			data = true;
		}
		else if (args[i].find("/id") == 0) {
			if (args[i] != "/id") {
				std::cerr << "ERROR: the argument [/vb] does not take any additional parameters.\n";
				return INVALID_SYNTAX;
			}

			id = true;
		}
		else if (args[i].find("/li") == 0) {
			if (args[i] != "/li") {
				std::cerr << "ERROR: the argument [/li] does not take any additional parameters.\n";
				return INVALID_SYNTAX;
			}

			std::cout << "requesting file list\n";
			result = getList(sock);
			if (result == NO_CONNECTION) {
				std::cout << "no connection or it was lost.\n";
			}
			else if (result != COM_SUCCESS) {
				std::cout << "request failed: " << result << "\n";
			}

			return PARSE_SUCCESS;
		}
		else if (args[i].find("/fn") == 0) {
			if (args[i] == "/fn") {
				std::cerr << "ERROR: the argument [/fn name] requires another parameter.\n";
				return INVALID_SYNTAX;
			}

			if ((sindex = args[i].find_first_of('"')) != -1) {
				filename = args[i].substr(0, args[i].find_last_of('"'));
				filename = filename.substr(sindex + 1);
			}
			else {
				trim(filename = args[i].substr(args[i].find_first_of(' ') + 1));
			}
		}
		else if (args[i].find("/vb") == 0) {
			if (args[i] != "/vb") {
				std::cerr << "ERROR: the argument [/vb] does not take any additional parameters.\n";
				return INVALID_SYNTAX;
			}

			showOut = true;
		}
		else {
			std::cerr << "ERROR: unknown argument: " << args[i] << "\n";
			return UNKNOWN_ARGUMENT;
		}
	}

	if (data) {
		result = getData(request, showOut, id, buflen, sock);
	}
	else {
		result = getFile(request, showOut, id, buflen, sock, downloadpath, filename);
	}

	if (result == NO_CONNECTION) {
		std::cout << "no connection or it was lost.\n";
	}
	else if (result != COM_SUCCESS) {
		std::cout << "request failed: " << result << "\n";
	}

	return PARSE_SUCCESS;
}

/* func: connect( const string&, csocket& )
 *  ret: PARSE_CODE
 *
 * desc: parses the connect command and uses extracted arguments to connect the sock to a server
*/
PARSE_CODE connect(const std::string& line, csocket& sock) {
	if (line.find("/?") != -1 || line == "/?") {
		std::cout << "usage:\n\tconnect [/ip ip] [/p port]\n\n";
		std::cout << "aliases:\n\tconnect, cn\n\n";
		std::cout << "desc:\n\t" << "This command is used to connect to a server. If [ip] or [port]\n\targuments are used they will be set as the default thereafter.\n\n";
		std::cout << "arguments:\n"
			<< "\t/ip ip\t\tSpecifies the ip of the server to connect to. If\n\t\t\tomitted, the ip will default to '127.0.0.1', the\n\t\t\tlast ip specified by this parameter, or the ip\n\t\t\tgiven by the command line arguments.\n"
			<< "\t/p port\t\tSpecifies the port of the server to connect to. If\n\t\t\tomitted, the port will default to '12345', the\n\t\t\tlast port specified by this parameter, or the port\n\t\t\tgiven by the command line arguments.\n"
			<< "\ntype a command followed by '?' to view more information about it.\n";
		return PARSE_SUCCESS;
	}


	std::string ip = sock.getIp(), port = sock.getPort();
	std::vector<std::string> args;

	args = split(line, "/");
	for (size_t i = 0; i < args.size(); ++i) {
		args[i] = trim(args[i]);
		if (args[i].find("/ip") == 0) {
			if (args[i] == "/ip") {
				std::cerr << "ERROR: the argument [/ip ip] requires another parameter.\nEnter send /? for proper usage information.\n";
				return INVALID_SYNTAX;
			}

			ip = trim(args[i].substr(3));
		}
		else if (args[i].find("/p") == 0) {
			int index = 2;
			if (args[i].find("/port") == 0) {
				index = 5;
			}
			if (args[i] == "/p" || args[i] == "/port") {
				std::cerr << "ERROR: the argument [/p port] requires another parameter.\nEnter send /? for proper usage information.\n";
				return INVALID_SYNTAX;
			}

			port = trim(args[i].substr(index));
		}
		else {
			std::cerr << "ERROR: unknown argument: " << args[i] << "\n";
			return UNKNOWN_ARGUMENT;
		}
	}

	sock.setIp(ip);
	sock.setPort(port);

	int result = sock.init();
	if (result != 0) {
		std::cout << "An error occured while initializing the socket.\nMake sure the ip and port are valid and try again.\n";
		return OTHER_ERROR;
	}
	result = sock.connectsocket();
	if (result != 0) {
		std::cout << "An error occured while connecting to the server.\nMake sure the ip and port are correct and try again.\n";
		return OTHER_ERROR;
	}

	std::cout << "successfully connected to the server: " << ip << "::" << port << "\n";

	return PARSE_SUCCESS;
}

/* func: disconnect( const string&, csocket& )
 *  ret: PARSE_CODE
 *
 * desc: parses the disconnect command and uses extracted arguments to disconnect the sock
*/
PARSE_CODE disconnect(const std::string& line, csocket& sock) {
	if (line.find("/?") != -1 || line == "/?") {
		std::cout << "usage:\n\tdisconnect\n\n";
		std::cout << "aliases:\n\tdisconnect, dc\n\n";
		std::cout << "desc:\n\t" << "This command is used to disconnect from a server.\n\n";
		std::cout << "arguments:\n"
			<< "\tThis command does not take any arguments.\n"
			<< "\ntype a command followed by '?' to view more information about it.\n";
		return PARSE_SUCCESS;
	}

	if (sock.isconnected()) {
		sock.disconnectsocket();
		std::cout << "successfully disconnected from the server.\n";
	}
	else {
		std::cout << "ERROR: the socket is already disconnected.\n";
	}

	return PARSE_SUCCESS;
}

/* func: input( const string&, csocket& )
 *  ret: PARSE_CODE
 * 
 * desc: determines command and passes rest of parsing to associated function
*/
PARSE_CODE input(const std::string& line, csocket& sock, const std::string& dnloadpath) {
	downloadpath = dnloadpath;
	size_t index = trim(line).find(' ');
	std::string command = trim(tolower(line.substr(0, index)));

	if (command == "send") {
		return send(trim(line.substr(index + 1)), sock);	//pass line without the command since it is no longer needed for parsing (also works if index == -1)
	}
	else if (command == "get") {
		return get(trim(line.substr(index + 1)), sock);		//see above comment
	}
	else if (command == "/?" || command == "?" || command == "help" || command == "/help") {
		std::cout << "server:\n"
			<< "\tport:\t\t" << sock.getPort() << "\n"
			<< "\tip:\t\t" << sock.getIp() << "\n"
			<< "\tconnected:\t" << ((sock.isconnected()) ? "true" : "false") << "\n";
		std::cout << "\ncommands:\n"
			<< "\thelp\t\tDisplays the help page with a list of all usable\n\t\t\tcommands.\n"
			<< "\tsend\t\tUsed to upload files onto the server.\n"
			<< "\tget\t\tUsed to download files from the server.\n"
			<< "\tconnect\t\tAttempts to connect to the specified server\n\t\t\tunless already connected to one.\n"
			<< "\tdisconnect\tDisconnects the user from the server.\n"
			<< "\tclear\t\tclears the screen.\n"
			<< "\texit\t\tExits the program. Ctrl + C has the same effect.\n"
			<< "\ntype a command followed by '?' to view more information about it.\n";
		return PARSE_SUCCESS;
	}
	else if (command == "") {
		return PARSE_SUCCESS;
	}
	else if (command == "connect" || command == "cn") {
		if (sock.isconnected() && line.find("?") == -1) {
			disconnect(line, sock);
		}
		if (trim(line) == command || index < 0) {
			return connect("", sock);
		}
		else {
			return connect(line.substr(index + 1), sock);
		}
	}
	else if (command == "disconnect" || command == "dc") {
		return disconnect(line, sock);
	}
	else if (command == "clear" || command == "cls" || command == "clr") {
		std::cout << std::string(100, '\n');
		return PARSE_SUCCESS;
	}
	else if (command == "exit" || command == "quit") {
		return TERMINATE;
	}

	std::cout << "ERROR: unknown command: " << command << "\ntype 'help' or '?' to view a list of commands.\n";
	return UNKNOWN_COMMAND;
}