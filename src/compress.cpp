#include "main.h"

// Сжатие в GZ
std::vector<char> compress_gz(const std::vector<char>& data, int level) {
    std::vector<char> compressed_data;
    z_stream zs{};
    deflateInit2(&zs, level, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);

    zs.avail_in = data.size();
    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));

    char outbuffer[32768];
    int ret;

    do {
        zs.avail_out = sizeof(outbuffer);
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        ret = deflate(&zs, Z_FINISH);

        compressed_data.insert(compressed_data.end(), outbuffer, outbuffer + sizeof(outbuffer) - zs.avail_out);
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        throw std::runtime_error("gzip compression failed.");
    }

    return compressed_data;
}

// Сжатие в XZ
// Реализация использует liblzma из исходников XZ Utils (собирается через FetchContent в CMakeLists.txt).
// Алгоритм аналогичен compress_gz: поточное сжатие через lzma_stream с буфером,
// формат — XZ-контейнер с CRC64 (стандартный для мобильных прошивок MTK).
std::vector<char> compress_xz(const std::vector<char>& data, int level) {
    std::vector<char> compressed_data;

    lzma_stream strm = LZMA_STREAM_INIT;

    // Инициализация энкодера: уровень сжатия 6 (баланс скорости и размера),
    // проверочная сумма CRC64 — стандарт для XZ-файлов в MTK-прошивках
    lzma_ret init_ret = lzma_easy_encoder(&strm, static_cast<uint32_t>(level), LZMA_CHECK_CRC64);
    if (init_ret != LZMA_OK) {
        throw std::runtime_error("xz compression: failed to initialize encoder (code " +
                                 std::to_string(init_ret) + ")");
    }

    strm.next_in  = reinterpret_cast<const uint8_t*>(data.data());
    strm.avail_in = data.size();

    char outbuffer[32768];
    lzma_ret ret;

    do {
        strm.avail_out = sizeof(outbuffer);
        strm.next_out  = reinterpret_cast<uint8_t*>(outbuffer);

        ret = lzma_code(&strm, LZMA_FINISH);

        if (ret != LZMA_OK && ret != LZMA_STREAM_END) {
            lzma_end(&strm);
            throw std::runtime_error("xz compression: encoding error (code " +
                                     std::to_string(ret) + ")");
        }

        compressed_data.insert(compressed_data.end(),
                               outbuffer,
                               outbuffer + sizeof(outbuffer) - strm.avail_out);
    } while (ret != LZMA_STREAM_END);

    lzma_end(&strm);
    return compressed_data;
}

// Сжатие в LZMA raw stream (старый формат, без XZ-контейнера).
// Используется когда при распаковке в meta_info было записано xz_subformat=lzma.
// lzma_alone_encoder производит формат совместимый с lzma_auto_decoder (используемым в decompress_xz).
std::vector<char> compress_lzma_raw(const std::vector<char>& data,
                                      uint8_t lc, uint8_t lp, uint8_t pb,
                                      uint32_t dict_size) {
    std::vector<char> compressed_data;

    lzma_stream strm = LZMA_STREAM_INIT;

    // lzma_alone_encoder: уровень 6, без XZ-обёртки — чистый LZMA raw stream.
    // Параметры preset=6 дают те же свойства (lc=3, lp=0, pb=2, dict=8MB),
    // что наблюдаются в оригинальных MTK-файлах (props byte = 0x5D).
    lzma_options_lzma opt_lzma;
    // Инициализируем preset=6 как базу, затем перезаписываем точными параметрами
    // из оригинального файла (lc, lp, pb, dict_size), считанными при распаковке.
    if (lzma_lzma_preset(&opt_lzma, 6)) {
        throw std::runtime_error("lzma raw compression: failed to set preset");
    }
    opt_lzma.lc        = lc;
    opt_lzma.lp        = lp;
    opt_lzma.pb        = pb;
    opt_lzma.dict_size = dict_size;

    lzma_ret init_ret = lzma_alone_encoder(&strm, &opt_lzma);
    if (init_ret != LZMA_OK) {
        throw std::runtime_error("lzma raw compression: failed to initialize encoder (code " +
                                 std::to_string(init_ret) + ")");
    }

    strm.next_in  = reinterpret_cast<const uint8_t*>(data.data());
    strm.avail_in = data.size();

    char outbuffer[32768];
    lzma_ret ret;

    do {
        strm.avail_out = sizeof(outbuffer);
        strm.next_out  = reinterpret_cast<uint8_t*>(outbuffer);

        ret = lzma_code(&strm, LZMA_FINISH);

        if (ret != LZMA_OK && ret != LZMA_STREAM_END) {
            lzma_end(&strm);
            throw std::runtime_error("lzma raw compression: encoding error (code " +
                                     std::to_string(ret) + ")");
        }

        compressed_data.insert(compressed_data.end(),
                               outbuffer,
                               outbuffer + sizeof(outbuffer) - strm.avail_out);
    } while (ret != LZMA_STREAM_END);

    lzma_end(&strm);
    return compressed_data;
}
