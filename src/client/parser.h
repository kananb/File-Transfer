#ifndef GUARD_PARSER_H
#define GUARD_PARSER_H

#include "csocket.h"

#include <string>
#include <vector>


enum PARSE_CODE {
	PARSE_SUCCESS, UNKNOWN_COMMAND, UNKNOWN_ARGUMENT, INVALID_SYNTAX, TERMINATE, OTHER_ERROR
};

PARSE_CODE input(const std::string&, csocket&, const std::string&);

#endif //GUARD_PARSER_H