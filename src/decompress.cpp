#include "main.h"

// Распаковка .gz файлов
std::vector<char> decompress_gz(const std::vector<char>& compressed_data) {
    std::vector<char> decompressed_data;
    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed_data.data()));
    stream.avail_in = compressed_data.size();

    if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) {
        throw std::runtime_error("Ошибка инициализации zlib");
    }

    char buffer[4096];
    do {
        stream.next_out = reinterpret_cast<Bytef*>(buffer);
        stream.avail_out = sizeof(buffer);

        int result = inflate(&stream, Z_NO_FLUSH);
        if (result == Z_STREAM_ERROR || result == Z_DATA_ERROR || result == Z_MEM_ERROR) {
            inflateEnd(&stream);
            throw std::runtime_error("Ошибка распаковки zlib");
        }

        decompressed_data.insert(decompressed_data.end(), buffer, buffer + (sizeof(buffer) - stream.avail_out));
    } while (stream.avail_out == 0);

    inflateEnd(&stream);
    return decompressed_data;
}

// Распаковка .xz файлов
std::vector<char> decompress_xz(const std::vector<char>& compressed_data) {
    std::vector<char> decompressed_data;
   /* lzma_stream stream = LZMA_STREAM_INIT;

    if (lzma_auto_decoder(&stream, UINT64_MAX, 0) != LZMA_OK) {
        throw std::runtime_error("Ошибка инициализации xz-utils");
    }

    stream.next_in = reinterpret_cast<const uint8_t*>(compressed_data.data());
    stream.avail_in = compressed_data.size();

    char buffer[4096];
    lzma_ret result;
    do {
        stream.next_out = reinterpret_cast<uint8_t*>(buffer);
        stream.avail_out = sizeof(buffer);

        result = lzma_code(&stream, LZMA_FINISH);
        if (result != LZMA_OK && result != LZMA_STREAM_END) {
            lzma_end(&stream);
            throw std::runtime_error("Ошибка распаковки xz-utils");
        }

        decompressed_data.insert(decompressed_data.end(), buffer, buffer + (sizeof(buffer) - stream.avail_out));
    } while (result != LZMA_STREAM_END);

    lzma_end(&stream);*/
    return decompressed_data;
}

