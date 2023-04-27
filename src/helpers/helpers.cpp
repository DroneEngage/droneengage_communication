#include <iostream>
#include <cctype>
#include <algorithm>
#include <sys/time.h>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include <cstdlib>
#include <cstdint>
#include <stdexcept>
#include "helpers.hpp"



uint32_t hex_string_to_uint32(const char* hex_str) {
    char* end_ptr;
    uint32_t hex_val = std::strtoul(hex_str, &end_ptr, 16);
    if (*end_ptr != '\0') {
        throw std::invalid_argument("Invalid hexadecimal string: " + std::string(hex_str));
    }
    return hex_val;
}

std::string str_tolower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), 
                   [](unsigned char c){ return std::tolower(c); } // correct
                  );
    return s;
}



std::vector<std::string> split_string_by_delimeter(const std::string& str, const char& delimeter)
{
    auto result = std::vector<std::string>{};
    auto ss = std::stringstream{str};

    for (std::string line; std::getline(ss, line, delimeter);)
        result.push_back(line);

    return result;
}



std::vector<std::string> split_string_by_newline(const std::string& str)
{
    return split_string_by_delimeter (str, '\n');
}


/**
 * @brief 
 * Remove comments from strings
 * http://www.cplusplus.com/forum/beginner/163419/
 * @param prgm 
 * @return std::string 
 */
std::string removeComments(std::string prgm) 
{ 
    int n = prgm.length(); 
    std::string res; 
  
    // Flags to indicate that single line and multpile line comments 
    // have started or not. 
    bool s_cmt = false; 
    bool m_cmt = false; 
  
  
    // Traverse the given program 
    for (int i=0; i<n; i++) 
    { 
        // If single line comment flag is on, then check for end of it 
        if (s_cmt == true && prgm[i] == '\n') 
            s_cmt = false; 
  
        // If multiple line comment is on, then check for end of it 
        else if  (m_cmt == true && prgm[i] == '*' && prgm[i+1] == '/') 
            m_cmt = false,  i++; 
  
        // If this character is in a comment, ignore it 
        else if (s_cmt || m_cmt) 
            continue; 
  
        // Check for beginning of comments and set the approproate flags 
        else if (prgm[i] == '/' && prgm[i+1] == '/') 
            s_cmt = true, i++; 
        else if (prgm[i] == '/' && prgm[i+1] == '*') 
            m_cmt = true,  i++; 
  
        // If current character is a non-comment character, append it to res 
        else  res += prgm[i]; 
    } 
    return res; 
} 


/**
 * @brief 
 *  returns true if field exist and of specified type
 * @param message Json object
 * @param field_name requested field name
 * @param field_type specified type
 * @return true if found and of specified type
 * @return false 
 */
bool validateField (const Json& message, const char *field_name, const Json::value_t& field_type)
{
    if (
        (message.contains(field_name) == false) 
        || (message[field_name].type() != field_type)
        ) 
    
        return false;

    return true;
}


/**
 * @brief Get the linux machine id object
 * 
 * @return std::string 
 */
std::string get_linux_machine_id ()
{
    FILE *f = fopen("/etc/machine-id", "r");
	if (!f) {
		return std::string();
	}
    char line[256]; 
    memset (line,0,255);
    char * read = fgets(line, 256, f);
    if (read!= NULL)
    {
        line[strlen(line)-1]=0; // remove "\n" from the read
        return std::string(line);
    }
    else
    {
        return std::string("");
    }
}
