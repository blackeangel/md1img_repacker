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
#include <stdexcept>
#include "zlib/zlib.h"       // Для распаковки .gz файлов
//#include <lzma.h>       // Для распаковки .xz файлов

const uint32_t MD1IMG_MAGIC1 = 0x58881688;
const uint32_t MD1IMG_MAGIC2 = 0x58891689;

// Заголовки .gz и .xz
const uint16_t GZ_HEADER=0x1F8B;
const uint64_t XZ_HEADER=0xFD377A585A00;

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

// Основная функция для распаковки файлов
void process_file(const std::string& input_path, const std::string& output_dir);
// Основная функция для упаковки файлов
void pack_files(const std::string &input_dir, const std::string &output_file);

// Проверка, заканчивается ли строка на определенный суффикс
bool ends_with(const std::string& value, const std::string& suffix);
// Проверка заголовка gzip файла (GZ_HEADER)
bool is_gz_format(const std::vector<char>& data);
// Проверка заголовка xz файла (XZ_HEADER)
bool is_xz_format(const std::vector<char>& data);
// Приведение строки к нижнему регистру
std::string to_lowercase(const std::string &str);
// Распаковка .xz файлов
std::vector<char> decompress_xz(const std::vector<char>& compressed_data);
// Распаковка .gz файлов
std::vector<char> decompress_gz(const std::vector<char>& compressed_data);
// Сжатие в GZ
std::vector<char> compress_gz(const std::vector<char>& input_data);
// Сжатие в XZ
std::vector<char> compress_xz(const std::vector<char>& input_data);
