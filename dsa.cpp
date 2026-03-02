//
// dsa.cpp
//

// this file contains the implementations of
// all the various functions related to certain
// data structures and algos (the headers for which
// have been defined in dsa.h)

#include "dsa.h"


bool fits_s32(std::string& str) {
    size_t i = 0;
    bool negative = false;

    // Handle optional sign
    if (str[i] == '+' || str[i] == '-') {
        negative = (str[i] == '-');
        i++;
        if (i == str.size())  // only "+" or "-"
            return false;
    }

    // Skip leading zeros
    while (i < str.size() && str[i] == '0') i++;

    size_t digits = str.size() - i;

    if (digits < 10) return true;
    if (digits > 10) return false;

    const char* limit = negative ? MAX_NEG : MAX_POS;

    for (size_t j = 0; j < 10; j++) {
        char c = str[i + j];
        if (c < limit[j]) return true;
        if (c > limit[j]) return false;
    }
    return true;
}