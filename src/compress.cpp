#include "main.h"

// Сжатие в GZ
std::vector<char> compress_gz(const std::vector<char>& data) {
    std::vector<char> compressed_data;
    z_stream zs{};
    deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);

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
std::vector<char> compress_xz(const std::vector<char>& data) {
    std::vector<char> compressed_data;
/*    lzma_stream strm = LZMA_STREAM_INIT;

    if (lzma_easy_encoder(&strm, LZMA_PRESET_DEFAULT, LZMA_CHECK_CRC64) != LZMA_OK) {
        throw std::runtime_error("xz compression failed to initialize.");
    }

    strm.next_in = reinterpret_cast<const uint8_t*>(data.data());
    strm.avail_in = data.size();

    char outbuffer[32768];
    lzma_ret ret;

    do {
        strm.avail_out = sizeof(outbuffer);
        strm.next_out = reinterpret_cast<uint8_t*>(outbuffer);
        ret = lzma_code(&strm, LZMA_FINISH);

        compressed_data.insert(compressed_data.end(), outbuffer, outbuffer + sizeof(outbuffer) - strm.avail_out);
    } while (ret == LZMA_OK);

    lzma_end(&strm);

    if (ret != LZMA_STREAM_END) {
        throw std::runtime_error("xz compression failed.");
    }
*/
    return compressed_data;
}
