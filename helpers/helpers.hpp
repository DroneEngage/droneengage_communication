#ifndef HELPERS_H_
#define HELPERS_H_


#include <iostream>

#include "json.hpp"
using Json = nlohmann::json;


/**
 * @brief Check if field exists in json and of a given type.
 * 
 * @param message 
 * @param field_name 
 * @param field_type 
 * @return true 
 * @return false 
 */
bool validateField (const Json& message, const char *field_name, Json::value_t field_type)
{
    if (
        (message.contains(field_name) == false) 
        || (message[field_name].type() != field_type)
        ) 
        return false;
    return true;
}

#endif