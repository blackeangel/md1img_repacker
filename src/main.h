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
#include <zlib.h>  // Для распаковки .gz файлов
#include <lzma.h>       // Для распаковки/упаковки .xz файлов (liblzma из исходников XZ Utils)

const uint32_t MD1IMG_MAGIC1 = 0x58881688;
const uint32_t MD1IMG_MAGIC2 = 0x58891689;

// Заголовки .gz и .xz
const uint16_t GZ_HEADER = 0x1F8B;
const uint64_t XZ_HEADER = 0xFD377A585A00ULL;

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
// Проверка заголовка xz файла (XZ_HEADER или LZMA raw stream)
bool is_xz_format(const std::vector<char>& data);
// Проверка, является ли файл именно LZMA raw stream (не XZ-контейнером)
bool is_lzma_raw_format(const std::vector<char>& data);
// Приведение строки к нижнему регистру
std::string to_lowercase(const std::string &str);
// Определение уровня сжатия GZ из заголовка (XFL байт)
int detect_gz_level(const std::vector<char>& data);
// Извлечение параметров LZMA raw stream (lc, lp, pb, dict_size)
void detect_lzma_params(const std::vector<char>& data, uint8_t& lc, uint8_t& lp, uint8_t& pb, uint32_t& dict_size);
// Определение уровня сжатия XZ-контейнера по размеру словаря LZMA2
int detect_xz_level(const std::vector<char>& data);
// Распаковка .xz файлов
std::vector<char> decompress_xz(const std::vector<char>& compressed_data);
// Распаковка .gz файлов
std::vector<char> decompress_gz(const std::vector<char>& compressed_data);
// Сжатие в GZ с заданным уровнем (1-9, default=Z_BEST_COMPRESSION=9)
std::vector<char> compress_gz(const std::vector<char>& data, int level = Z_BEST_COMPRESSION);
// Сжатие в XZ-контейнер (современный формат) с заданным уровнем (0-9, default=6)
std::vector<char> compress_xz(const std::vector<char>& data, int level = 6);
// Сжатие в LZMA raw stream с точными параметрами из оригинального файла
std::vector<char> compress_lzma_raw(const std::vector<char>& data,
                                     uint8_t lc = 3, uint8_t lp = 0, uint8_t pb = 2,
                                     uint32_t dict_size = 8u * 1024u * 1024u);
