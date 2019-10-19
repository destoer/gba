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
#include "../fmt/format.h"

#define UNUSED(X) ((void)X)
void read_file(std::string filename, std::vector<uint8_t> &buf);

inline bool is_set(int reg, int bit)
{
	return ((reg >> bit) & 1);
}
