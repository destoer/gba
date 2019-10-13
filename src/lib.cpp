#include "headers/lib.h"
#include <fstream>


std::vector<uint8_t> read_file(std::string filename)
{
    std::ifstream fp(filename,std::ios::binary);

    if(!fp)
    {
        std::cout << "failed to open file: " << filename;
        exit(1);
    }

    // get file size
    size_t size;

    fp.seekg(0,std::ios::end);
    size = fp.tellg();
    fp.seekg(0,std::ios::beg);

    fp.clear();

    std::vector<uint8_t> v(size);

    fp.read((char*)v.data(),size);


    return v;

}

