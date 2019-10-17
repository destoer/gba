#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define UNUSED(X) ((void)X)
void read_file(std::string filename, std::vector<uint8_t> &buf);