#include "main.h"
// Проверка, заканчивается ли строка на определенный суффикс
bool ends_with(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Проверка заголовка gzip файла (GZ_HEADER)
bool is_gz_format(const std::vector<char>& data) {
    return data.size() > 2 &&
           (static_cast<uint8_t>(data[0]) << 8 | static_cast<uint8_t>(data[1])) == GZ_HEADER;
}

// Проверка заголовка xz файла (XZ_HEADER)
bool is_xz_format(const std::vector<char>& data) {
    return data.size() > 6 &&
           (static_cast<uint64_t>(data[0]) << 40 |
            static_cast<uint64_t>(data[1]) << 32 |
            static_cast<uint64_t>(data[2]) << 24 |
            static_cast<uint64_t>(data[3]) << 16 |
            static_cast<uint64_t>(data[4]) << 8  |
            static_cast<uint64_t>(data[5])) == XZ_HEADER;
}

// Приведение строки к нижнему регистру
std::string to_lowercase(const std::string &str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}
