#ifndef GUARD_FILE_H
#define GUARD_FILE_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>


class File {
public:
	const static std::size_t npos = -1;

	File(const std::string&);

	void read();
	void write(const std::vector<char>&, bool = 0);
	void write(const std::string&, bool = 0);
	void write(const char*, bool = 0);
	void clear();

	const std::string& path() const;
	const std::string& name() const;
	std::size_t size() const;
	const std::vector<char>& contents() const;
	std::vector<char> contents(std::size_t, std::size_t = npos) const;

	void path(const std::string&);
	void name(const std::string&);

private:
	std::string filePath;
	std::string fileName;
	std::size_t fileSize;
	std::vector<char> fileContents;

	std::ofstream outFile;
	std::ifstream inFile;
};

#endif //GUARD_FILE_H