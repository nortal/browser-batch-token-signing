#include "HashListParser.h"
#include "BinaryUtils.h"
#include "Logger.h"

using namespace std;

std::vector<std::vector<unsigned char>> HashListParser::parse(std::string hashList)
{
	vector<vector<unsigned char>> hashes;
	int hashPos = 0;
	string hashString = getNextHash(hashList, hashPos);
	while (hashString != "") {
		_log("Received hash: %s", hashString.c_str());
		vector<unsigned char> hash = BinaryUtils::hex2bin(hashString);
		hashes.push_back(hash);
		hashString = getNextHash(hashList, hashPos);
	}
	return hashes;
}

string HashListParser::getNextHash(string hashList, int& position)
{
	const char separator = ',';
	string result;
	bool found = false;

	// initialize search
	const char* str = hashList.c_str();
	str += position;
	
	// skip separator in the beginning of search
	if (*str == separator)
	{
		str++;
		position++;
	}
	
	// store the current position (beginning of substring)
	const char *begin = str;
	
	// while separator not found and not at end of string..
	while (*str != separator && *str)
	{
		// ..go forward in the string.
		str++;
		position++;
	}
	
	// return what we've got, which is either empty string or a hash string
	result = std::string(begin, str);
	return result;
}
