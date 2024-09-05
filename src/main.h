#ifndef MD1IMG_REPACKER_MAIN_H
#define MD1IMG_REPACKER_MAIN_H

#endif //MD1IMG_REPACKER_MAIN_H
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <sstream>
#include <cstring>
#include <cstdint>

const uint32_t MD1IMG_MAGIC1 = 0x58881688;
const uint32_t MD1IMG_MAGIC2 = 0x58891689;

#pragma pack(push, 1)
struct Header {
    uint32_t magic1;
    uint32_t data_size;
    char name[32];
    uint32_t base;
    uint32_t mode;
    uint32_t magic2;
    uint32_t data_offset;
    uint32_t reserved[5];
};
#pragma pack(pop)
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <sstream>
#include <cstring>
#include <cstdint>

void write_packed_file(const std::string& input_dir, const std::string& output_file_path);
std::unordered_map<std::string, std::string> read_file_mapping(const std::string& path);
void process_file(const std::string& input_path, const std::string& output_dir, const std::unordered_map<std::string, std::string>& file_mapping);
