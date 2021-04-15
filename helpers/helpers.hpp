#ifndef HELPERS_H_
#define HELPERS_H_

#include <iostream>
#include <cctype>
#include <algorithm>
#include <sys/time.h>
#include <vector>
#include <sstream>

#include "json.hpp"
using Json = nlohmann::json;

static inline uint64_t get_time_usec()
{
	static struct timeval _time_stamp;
	gettimeofday(&_time_stamp, NULL);
	return _time_stamp.tv_sec*1000000 + _time_stamp.tv_usec;
};

inline int wait_time_nsec (const time_t& seconds, const long& nano_seconds)
{
	struct timespec _time_wait, tim2;
	_time_wait.tv_sec = seconds;
	_time_wait.tv_nsec = nano_seconds;
	
	return nanosleep(&_time_wait, &tim2);
}

extern std::string str_tolower(std::string s);

extern std::vector<std::string> split_string_by_delimeter(const std::string& str, const char& delimeter);

extern std::vector<std::string> split_string_by_newline(const std::string& str);

extern std::string removeComments(std::string prgm);

extern bool validateField (const Json& message, const char *field_name, const Json::value_t& field_type);


#endif