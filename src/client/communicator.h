#ifndef GUARD_COMMUNICATOR_H
#define GUARD_COMMUNICATOR_H

#include "csocket.h"

#include <string>


enum COM_CODE {
	COM_SUCCESS, NO_CONNECTION, BAD_REPLY, RECV_FAILED, MISSING_DATA, EXTRA_DATA
};

const static int DEFAULT_BUFLEN = 134217728;


COM_CODE sendFile(const std::string&, const std::string&, bool, int, csocket&);

COM_CODE getList(csocket&);
COM_CODE getData(const std::string&, bool, bool, int, csocket&);
COM_CODE getFile(const std::string&, bool, bool, int, csocket&, const std::string&, const std::string&);

#endif //GUARD_COMMUNICATOR_H