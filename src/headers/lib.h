#pragma once
#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <functional>
#include <numeric>
#include <limits>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <SDL2/SDL.h>
#include "../fmt/format.h"

#define UNUSED(X) ((void)X)
void read_file(std::string filename, std::vector<uint8_t> &buf);

inline bool is_set(uint64_t reg, int bit)
{
	return ((reg >> bit) & 1);
}

inline uint64_t set_bit(uint64_t v,int bit)
{
    return (v |= (1 << bit));
}


inline uint64_t deset_bit(uint64_t v,int bit)
{
    return (v &= ~(1 << bit));
}

// std::rotr and std::rotl in c++20 probs should be used
// for now https://stackoverflow.com/questions/776508/best-practices-for-circular-shift-rotate-operations-in-c

inline uint32_t rotl(uint32_t n, unsigned int c)
{
    const unsigned int mask = (CHAR_BIT*sizeof(n) - 1);  
    c &= mask;
    return (n<<c) | (n>>( (-c)&mask ));
}

inline uint32_t rotr(uint32_t n, unsigned int c)
{
    const unsigned int mask = (CHAR_BIT*sizeof(n) - 1);
    c &= mask;
    return (n>>c) | (n<<( (-c)&mask ));
}

 // https://stackoverflow.com/questions/5814072/sign-extend-a-nine-bit-number-in-c
inline int64_t sign_extend(int64_t x, int64_t b)
{
    /* generate the sign bit mask. 'b' is the extracted number of bits */
    int m = 1U << (b - 1);  

    /* Transform a 'b' bits unsigned number 'x' into a signed number 'r' */
    int r = (x ^ m) - m; 

    return r;
}
