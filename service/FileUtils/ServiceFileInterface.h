#ifndef SERVICE_SERVICEFILEINTERFACE_H
#define SERVICE_SERVICEFILEINTERFACE_H

#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <openssl/sha.h>
#include "../common/myconcepts.h"

#include <sstream>
#include <iomanip>


using buf64 = segvec<64>;

template<unsigned long segment_size>
requires IsDiv<segment_size, SHA256size>
std::istream &operator>>(std::istream &source_, segvec<segment_size> &buf) {
    while (!source_.eof()) {
        auto curr_s = segment<segment_size>();
        curr_s.fill(0);
        for (int j = 0; j < segment_size; ++j) {
            if (source_.eof() || source_.peek() == -1) {
                curr_s[j] = 23;//ETB symbol
                if (j == 0) {
                    return source_;
                }
                buf.push_back(curr_s);
                return source_;
            }
            curr_s[j] = source_.get();
        }
        buf.push_back(curr_s);
    }
    return source_;
}

template<unsigned long segment_size>
requires IsDiv<segment_size, SHA256size>
std::ostream &operator<<(std::ostream &target_, const segvec<segment_size> &buf) {
    for (auto &elem: buf) {
        for (auto &symbol: elem) {
            if (symbol == 23) {
                return target_;
            }
            target_.put(symbol);
        }
    }
    return target_;
}


#endif //SERVICE_SERVICEFILEINTERFACE_H
