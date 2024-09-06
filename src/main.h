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
#include <regex>


const uint32_t MD1IMG_MAGIC1 = 0x58881688;
const uint32_t MD1IMG_MAGIC2 = 0x58891689;

const std::string FILE_MAP_MARKER = "md1_file_map";
const std::vector<uint8_t> FILE_MAP_SIGNATURE = {0x6D, 0x64, 0x31, 0x5F, 0x66, 0x69, 0x6C, 0x65, 0x5F, 0x6D, 0x61, 0x70};


#pragma pack(push, 1)
struct Header {
    uint32_t magic1;         // 4 bytes
    uint32_t data_size;      // 4 bytes
    char name[32];           // 32 bytes
    uint32_t base;           // 4 bytes
    uint32_t mode;           // 4 bytes
    uint32_t magic2;         // 4 bytes
    uint32_t data_offset;    // 4 bytes
    uint32_t reserved[6];    // 24 bytes
    uint8_t reserved2[432];  // 432 bytes, заполненные значением 0xFF
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
std::unordered_map<std::string, std::string> read_file_mapping(const std::string& path, std::streampos map_offset);
std::streampos find_file_map_offset(const std::string& path);
void process_file(const std::string& input_path, const std::string& output_dir);
void unpack(const std::string& input_path, const std::string& output_dir);
void pack_files(const std::string &input_dir, const std::string &output_file);