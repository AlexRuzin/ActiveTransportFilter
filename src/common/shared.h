#pragma once

#ifndef __cplusplus
#error "shared.h requires a C++ compiler"
#endif //__cplusplus

#if __cplusplus < 202002L
#error "shared.h requires C++20 or later"
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <filesystem>
#include <vector>

#include <stdint.h>

namespace shared {

inline bool IsFileExists(std::string filepath)
{
    const std::filesystem::path fsPath(filepath);
    return std::filesystem::exists(fsPath);
}

inline std::vector<std::string> SplitStringByDelimiter(const std::string str, char delimiter) {
    std::vector<std::string> out;

    std::istringstream tokenStream(str);

    std::string currToken;
    while (std::getline(tokenStream, currToken, delimiter)) {
        out.push_back(currToken);
    }

    return out;
}

//
// Convert a string to int, and return if it is valid or not
//  Returns true if string
//
inline bool ConvertStringToInt(const std::string &in, uint32_t &out)
{
    std::istringstream iss(in);
    iss >> out;

    return iss.eof() && !iss.fail();
}

//
// Convert a string to an IPv4 address, return as uint32_t in little endian
//  If it's not a valid address, return false
//
inline bool ParseStringToIpv4(const std::string &ip, uint32_t &ipOut)
{
    ipOut = 0;

    // Tokenize string first
    const std::vector<std::string> tokenized = SplitStringByDelimiter(ip, '.');
    if (tokenized.size() != 3) {
        return false;
    }

    for (std::vector<std::string>::const_iterator i = tokenized.begin(); i != tokenized.end(); i++) {
        uint32_t out = -1;

        // String must be an int, and only a single byte
        if (!ConvertStringToInt(*i, out) || out & 0xffffff00) {
            return false;
        }

        ipOut |= (uint8_t)(out & 0xffffff00) << (i - tokenized.begin());
    }

    return true;
}

}