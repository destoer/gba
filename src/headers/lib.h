#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <functional>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../fmt/format.h"

#define UNUSED(X) ((void)X)
void read_file(std::string filename, std::vector<uint8_t> &buf);

inline bool is_set(int reg, int bit)
{
	return ((reg >> bit) & 1);
}

/*
#define set_bit(dest,bit) ((dest) |= (1 << bit))

// deset (that is set to zero a bit)
#define deset_bit(dest,bit) ((dest) &= ~(1 << bit)) 

*/

static inline uint32_t set_bit(uint32_t v,int bit)
{
    return (v |= (1 << bit));
}


static inline uint32_t deset_bit(uint32_t v,int bit)
{
    return (v &= ~(1 << bit));
}

// std::rotr and std::rotl in c++20 probs should be used
// for now https://stackoverflow.com/questions/776508/best-practices-for-circular-shift-rotate-operations-in-c

static inline uint32_t rotl(uint32_t n, unsigned int c)
{
    const unsigned int mask = (CHAR_BIT*sizeof(n) - 1);  
    c &= mask;
    return (n<<c) | (n>>( (-c)&mask ));
}

static inline uint32_t rotr(uint32_t n, unsigned int c)
{
    const unsigned int mask = (CHAR_BIT*sizeof(n) - 1);
    c &= mask;
    return (n>>c) | (n<<( (-c)&mask ));
}

