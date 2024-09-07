#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <sstream>
#include <cstring>
#include <cstdint>
#include <algorithm>

const uint32_t MD1IMG_MAGIC1 = 0x58881688;
const uint32_t MD1IMG_MAGIC2 = 0x58891689;

const std::string FILE_MAP_MARKER = "md1_file_map";

#pragma pack(push, 1)
struct Header {
    uint32_t magic1;         // 4 bytes
    uint32_t data_size;      // 4 bytes
    char name[32];           // 32 bytes
    uint32_t base;           // 4 bytes
    uint32_t mode;           // 4 bytes
    uint32_t magic2;         // 4 bytes
    uint32_t data_offset;    // 4 bytes
    uint32_t hdr_version;    // 4 bytes
    uint32_t img_type;       // 4 bytes
    uint32_t img_list_end;   // 4 bytes
    uint32_t align_size;     // 4 bytes
    uint32_t dsize_extend;   // 4 bytes
    uint32_t maddr_extend;   // 4 bytes
    uint8_t reserved[432];  // 432 bytes, заполненные значением 0xFF
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

// Основная функция для распаковки файлов
void process_file(const std::string& input_path, const std::string& output_dir);
// Основная функция для упаковки файлов
void pack_files(const std::string &input_dir, const std::string &output_file);