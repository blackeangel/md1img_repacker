#include "main.h"

std::unordered_map<std::string, std::string> read_file_mapping(const std::string& path) {
    std::unordered_map<std::string, std::string> mapping;
    std::ifstream input_file(path, std::ios::binary | std::ios::ate);
    if (!input_file) {
        std::cerr << "Error opening file: " << path << std::endl;
        return mapping;
    }

    std::streamsize size = input_file.tellg();
    input_file.seekg(size - 1024); // Предполагаем, что последний 1KB содержит нужные данные
    std::string buffer(1024, '\0');
    input_file.read(buffer.data(), 1024);

    size_t pos = buffer.find_last_not_of('\0');
    if (pos != std::string::npos) {
        buffer.erase(pos + 1);
    }

    std::stringstream ss(buffer);
    std::string line;
    while (std::getline(ss, line)) {
        auto delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos) {
            std::string key = line.substr(0, delimiter_pos);
            std::string value = line.substr(delimiter_pos + 1);
            mapping[key] = value;
        }
    }

    return mapping;
}

void process_file(const std::string& input_path, const std::string& output_dir, const std::unordered_map<std::string, std::string>& file_mapping) {
    std::filesystem::path input_file_path(input_path);
    std::error_code ec;
    auto file_size = std::filesystem::file_size(input_file_path, ec);
    if (ec) {
        std::cerr << "Error getting file size: " << ec.message() << std::endl;
        return;
    }

    std::ifstream input_file(input_path, std::ios::binary);
    if (!input_file) {
        std::cerr << "Error opening file: " << input_path << std::endl;
        return;
    }

    size_t offset = 0;
    std::filesystem::create_directories(output_dir); // Создаем выходную папку, если она не существует

    while (offset < file_size) {
        input_file.seekg(offset, std::ios::beg);
        Header header;
        input_file.read(reinterpret_cast<char*>(&header), sizeof(Header));

        if (header.magic1 != MD1IMG_MAGIC1 || header.magic2 != MD1IMG_MAGIC2) {
            break;
        }

        std::string name(header.name, strnlen(header.name, sizeof(header.name)));
        std::cout << "Found " << name << "@0x" << std::hex << header.base << ",0x" << header.data_size << std::dec << std::endl;

        std::string output_name = output_dir + "/" + name + ".bin";
        if (file_mapping.find(name) != file_mapping.end()) {
            output_name = output_dir + "/" + file_mapping.at(name);
        }

        offset += header.data_offset;
        input_file.seekg(offset, std::ios::beg);

        std::vector<char> data(header.data_size);
        input_file.read(data.data(), header.data_size);

        std::ofstream output_file(output_name, std::ios::binary);
        if (!output_file) {
            std::cerr << "Error opening output file for writing: " << output_name << std::endl;
            return;
        }
        output_file.write(data.data(), header.data_size);
        std::cout << name << " written to " << output_name << std::endl;

        offset += header.data_size;
        if (offset % 16 != 0) {
            offset += 16 - (offset % 16);
        }
    }
}