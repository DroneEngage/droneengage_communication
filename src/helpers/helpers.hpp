#ifndef HELPERS_H_
#define HELPERS_H_

#include <iostream>
#include <cctype>
#include <algorithm>
#include <sys/time.h>
#include <vector>
#include <sstream>

#include "json_nlohmann.hpp"
using Json_de = nlohmann::json;

#define SEC_M500   500000l
#define SEC_1     1000000l
#define SEC_2     2000000l
#define SEC_3     3000000l
#define SEC_4     4000000l
#define SEC_5     5000000l
#define SEC_6     6000000l
#define SEC_7     7000000l
#define SEC_8     8000000l
#define SEC_9     9000000l
#define SEC_10   10000000l
#define SEC_15   15000000l
#define SEC_20   20000000l
#define SEC_30   30000000l

std::string get_time_string();

uint64_t get_time_usec();
void time_register(uint64_t& time_box);
bool time_passed_usec(const uint64_t& time_box, const uint64_t diff_usec);
bool time_less_usec(const uint64_t& time_box, const uint64_t diff_usec);

bool time_passed_register_usec(uint64_t& time_box, const uint64_t diff_usec);

int wait_time_nsec (const time_t& seconds, const long& nano_seconds);

extern uint32_t hex_string_to_uint32(const char* hex_str);

extern std::string str_tolower(std::string s);

extern std::vector<std::string> split_string_by_delimeter(const std::string& str, const char& delimeter);

extern std::vector<std::string> split_string_by_newline(const std::string& str);

extern std::string removeComments(std::string prgm);

extern bool validateField (const Json_de& message, const char *field_name, const Json_de::value_t& field_type);

extern std::string get_linux_machine_id ();

extern std::string convertMacAddressToString(const std::vector<int>& mac);

void saveBinaryToFile(const char* binary_message, size_t binary_length, const std::string& file_path);

#endif