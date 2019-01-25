#include "File.h"
#include <ios>
#include <direct.h>
#include <mutex>


File::File(const std::string& path) :
	filePath(path.substr(0, ((path.find("\\") != -1) ? path.find_last_of("\\") : path.find_last_of("/")) + 1)),
	fileName(path.substr(((path.find("\\") != -1) ? path.find_last_of("\\") : path.find_last_of("/")) + 1)),
	fileSize(0), fileContents(), outFile(), inFile() {
	_mkdir(filePath.c_str());

	inFile.open(path, std::ios::binary | std::ios::ate);

	fileSize = (int)inFile.tellg();
	if (inFile.tellg() < 0) fileSize = 0;

	inFile.close();
	inFile.clear();

	if (fileSize > 0) read();
}



void File::read() {
	inFile.open(filePath + fileName, std::ios::binary);
	fileContents.resize(fileSize);
	inFile.seekg(0, std::ios::beg);
	inFile.read(&fileContents[0], fileSize);

	inFile.close();
	inFile.clear();
}

std::mutex mut;
void File::write(const std::vector<char>& data, bool append) {
	mut.lock();
	outFile.open(filePath + fileName, std::ios::binary | (append ? std::ios::app : std::ios::beg));
	outFile.write(&data[0], data.size());
	fileSize = data.size();
	outFile.close();
	outFile.clear();

	fileContents = data;
	fileSize = fileContents.size();
	mut.unlock();
}

void File::write(const std::string& data, bool append) {
	write(&data[0], append);
}

void File::write(const char* data, bool append) {
	mut.lock();
	outFile.open(filePath + fileName, std::ios::binary | (append ? std::ios::app : std::ios::beg));
	outFile.write(data, strlen(data));
	outFile.close();
	outFile.clear();

	fileContents = std::vector<char>(data, data + strlen(data));
	fileSize = fileContents.size();
	mut.unlock();
}

void File::clear() {
	outFile.open(filePath + fileName, std::ios::trunc);
	outFile.close();
	outFile.clear();

	fileContents.clear();
	fileSize = 0;
}



const std::string& File::path() const {
	return filePath;
}

const std::string& File:: name() const {
	return fileName;
}

std::size_t File::size() const {
	return (std::size_t)fileSize;
}

const std::vector<char>& File::contents() const {
	return fileContents;
}

std::vector<char> File::contents(std::size_t beg, std::size_t end) const {
	if (end == npos) {
		end = fileSize;
	}

	std::vector<char> ret(fileContents.begin() + beg, fileContents.begin() + end);
	return ret;
}


void File::path(const std::string& fpath) {
	filePath = fpath;
}

void File::name(const std::string& fname) {
	fileName = fname;
}