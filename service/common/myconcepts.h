#ifndef SERVICE_MYCONCEPTS_H
#define SERVICE_MYCONCEPTS_H

#include <concepts>
#include <array>
#include <vector>

static const int SHA256size = 32; //SHA256_DIGEST_LENGTH;


template<unsigned short seg, unsigned short div>
concept IsDiv = seg % div == 0;

template<unsigned long segment_size> requires IsDiv<segment_size, SHA256size>
using segment = std::array<char, segment_size>;
template<unsigned long segment_size> requires IsDiv<segment_size, SHA256size>
using segvec = std::vector<segment<segment_size>>;


#endif //SERVICE_MYCONCEPTS_H
