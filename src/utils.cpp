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
    // Файлы с расширением .xz в прошивках MTK бывают двух форматов:
    //
    // 1. XZ-контейнер — magic: FD 37 7A 58 5A 00
    //    Современный формат, обёртка над LZMA2.
    //
    // 2. Legacy LZMA raw stream — начинается с байта свойств (props byte),
    //    за которым следует 4-байтовый размер словаря и 8-байтовый размер данных.
    //    Никакого фиксированного magic нет, но props byte однозначно валидируется:
    //    props = lc + lp*9 + pb*45, где lc<=8, lp<=4, pb<=4 → props <= 224 (0xE0).
    //    Пример из реального файла: 5D 00 00 00 01 → pb=2, lp=0, lc=3 — валидный LZMA.
    //
    // lzma_auto_decoder (используемый в decompress_xz) прозрачно обрабатывает оба формата,
    // поэтому достаточно расширить детектирование здесь.
    //
    // Промежуточный каст через uint8_t обязателен: char знаковый, 0xFD как char = -3,
    // прямой каст в uint64_t даёт sign extension → 0xFFFFFFFFFFFFFFFD вместо 0xFD.

    if (data.size() < 6) return false;

    // --- Проверка XZ-контейнера ---
    uint64_t header =
        static_cast<uint64_t>(static_cast<uint8_t>(data[0])) << 40 |
        static_cast<uint64_t>(static_cast<uint8_t>(data[1])) << 32 |
        static_cast<uint64_t>(static_cast<uint8_t>(data[2])) << 24 |
        static_cast<uint64_t>(static_cast<uint8_t>(data[3])) << 16 |
        static_cast<uint64_t>(static_cast<uint8_t>(data[4])) << 8  |
        static_cast<uint64_t>(static_cast<uint8_t>(data[5]));
    if (header == XZ_HEADER) return true;

    // --- Проверка legacy LZMA raw stream ---
    // Минимальный размер: 1 (props) + 4 (dict size) + 8 (uncompressed size) = 13 байт
    if (data.size() < 13) return false;
    uint8_t props = static_cast<uint8_t>(data[0]);
    // Максимальное валидное значение props: lc=8, lp=4, pb=4 → 8 + 4*9 + 4*45 = 224
    if (props > 224) return false;
    uint8_t pb  = props / 45;
    uint8_t rem = props % 45;
    uint8_t lp  = rem / 9;
    uint8_t lc  = rem % 9;
    return pb <= 4 && lp <= 4 && lc <= 8;
}


// Проверяет, является ли данные LZMA raw stream (старый формат без XZ-контейнера).
// Используется при распаковке, чтобы сохранить субформат в meta_info
// и при обратной упаковке выбрать правильный энкодер.
bool is_lzma_raw_format(const std::vector<char>& data) {
    if (data.size() < 13) return false;           // 1 props + 4 dict + 8 size
    uint8_t props = static_cast<uint8_t>(data[0]);
    if (props > 224) return false;                // lc<=8, lp<=4, pb<=4 → max=224
    uint8_t pb  = props / 45;
    uint8_t rem = props % 45;
    uint8_t lp  = rem / 9;
    uint8_t lc  = rem % 9;
    return pb <= 4 && lp <= 4 && lc <= 8;
}

// Определяет уровень сжатия GZ по полю XFL (байт 8 заголовка gzip).
// XFL=0x02 → zlib выставил максимальное сжатие (level 9)
// XFL=0x04 → zlib выставил минимальное (level 1)
// XFL=0x00 → уровень не указан (zlib default = level 6)
// Значение приблизительное: zlib не кодирует уровни 2-8 в XFL.
int detect_gz_level(const std::vector<char>& data) {
    if (data.size() < 9) return 6;
    uint8_t xfl = static_cast<uint8_t>(data[8]);
    if (xfl == 0x02) return 9;
    if (xfl == 0x04) return 1;
    return 6;
}

// Извлекает точные параметры LZMA raw stream из заголовка (первые 5 байт).
// Формат: [props(1)] [dict_size LE u32(4)] [uncompressed_size LE u64(8)]
// props = lc + lp*9 + pb*45  — однозначно раскладывается обратно.
// dict_size хранится побайтово в little-endian.
void detect_lzma_params(const std::vector<char>& data,
                         uint8_t& lc, uint8_t& lp, uint8_t& pb,
                         uint32_t& dict_size) {
    uint8_t props = static_cast<uint8_t>(data[0]);
    pb  = props / 45;
    uint8_t rem = props % 45;
    lp  = rem / 9;
    lc  = rem % 9;
    dict_size =
        static_cast<uint32_t>(static_cast<uint8_t>(data[1]))        |
        static_cast<uint32_t>(static_cast<uint8_t>(data[2])) << 8   |
        static_cast<uint32_t>(static_cast<uint8_t>(data[3])) << 16  |
        static_cast<uint32_t>(static_cast<uint8_t>(data[4])) << 24;
}

// Определяет уровень сжатия XZ-контейнера по размеру словаря LZMA2.
// Парсит заголовок блока: смещение 12 от начала файла.
// При любой неожиданной структуре возвращает 6 (безопасный default).
int detect_xz_level(const std::vector<char>& data) {
    // Структура: 6 magic + 2 stream_flags + 4 CRC32 = 12 байт → начало блока
    // Минимум нужно 17 байт: 12 + 1(bh_size) + 1(flags) + 1(filter_id) + 1(props_size) + 1(lzma2_props)
    if (data.size() < 17) return 6;

    uint8_t bh_flags = static_cast<uint8_t>(data[13]);

    // Биты 6,7: наличие полей Compressed/Uncompressed Size (VLI переменной длины).
    // Если хотя бы одно присутствует, смещение списка фильтров сдвигается — пропускаем.
    if (bh_flags & 0xC0) return 6;

    // При отсутствии обоих полей список фильтров начинается с байта 14.
    // LZMA2 Filter ID = 0x21 (1-байтовый VLI)
    if (static_cast<uint8_t>(data[14]) != 0x21) return 6;
    // Size of Properties для LZMA2 = 0x01 (1 байт)
    if (static_cast<uint8_t>(data[15]) != 0x01) return 6;

    uint8_t d = static_cast<uint8_t>(data[16]); // LZMA2 properties byte

    // Декодируем размер словаря по спецификации XZ:
    //   d == 40         → dict = UINT32_MAX (специальное значение)
    //   d чётный        → dict = 2 << (d/2 + 10)   = 1 << (d/2 + 11)
    //   d нечётный      → dict = 3 << (d/2 + 9)    (целочисленное деление)
    uint64_t dict;
    if (d == 40) {
        dict = UINT32_MAX;
    } else if (d % 2 == 0) {
        dict = uint64_t(2) << (d / 2 + 10);
    } else {
        dict = uint64_t(3) << (d / 2 + 9);
    }

    // Маппинг dict_size → preset level (при совпадении двух уровней берём больший)
    // Level 0:1  1:2  2:3  3-4:4  5-6:6  7:8  8:9  9:10 (МБ)
    if (dict <= (uint64_t(1) << 18)) return 0;  // ≤256K
    if (dict <= (uint64_t(1) << 20)) return 1;  // ≤1M
    if (dict <= (uint64_t(1) << 21)) return 2;  // ≤2M
    if (dict <= (uint64_t(1) << 22)) return 4;  // ≤4M  (уровни 3 и 4 оба = 4M, берём 4)
    if (dict <= (uint64_t(1) << 23)) return 6;  // ≤8M  (уровни 5 и 6 оба = 8M, берём 6)
    if (dict <= (uint64_t(1) << 24)) return 7;  // ≤16M
    if (dict <= (uint64_t(1) << 25)) return 8;  // ≤32M
    return 9;                                    // >32M
}
// Приведение строки к нижнему регистру
std::string to_lowercase(const std::string &str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}
