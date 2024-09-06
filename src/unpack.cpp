#include "main.h"

// Функция для поиска "md1_file_map" с конца файла
std::streampos find_file_map_offset(const std::string &path) {
    const size_t buffer_size = 16384;  // Буфер 16КБ
    std::ifstream input_file(path, std::ios::binary | std::ios::ate);
    if (!input_file) {
        std::cerr << "Error opening file: " << path << std::endl;
        return -1;
    }

    std::streampos file_size = input_file.tellg();
    if (file_size < buffer_size) {
        std::cerr << "File too small to contain md1_file_map" << std::endl;
        return -1;
    }

    std::vector<char> buffer(buffer_size);
    std::streampos pos = file_size;

    while (pos > 0) {
        size_t read_size = std::min(buffer_size, static_cast<size_t>(pos));
        pos = pos - static_cast<std::streampos>(read_size);
        input_file.seekg(pos);
        input_file.read(buffer.data(), read_size);

        // Ищем строку "md1_file_map" в буфере
        std::string data(buffer.data(), read_size);
        size_t found = data.rfind(FILE_MAP_MARKER);
        if (found != std::string::npos) {
            return pos + static_cast<std::streampos>(found);
        }
    }

    std::cerr << "md1_file_map not found" << std::endl;
    return -1;
}

// Функция для чтения маппинга файлов после нахождения "md1_file_map"
std::unordered_map<std::string, std::string> read_file_mapping(const std::string &path, std::streampos map_offset) {
    std::unordered_map<std::string, std::string> mapping;
    std::ifstream input_file(path, std::ios::binary);
    if (!input_file) {
        std::cerr << "Error opening file: " << path << std::endl;
        return mapping;
    }

    input_file.seekg(map_offset);
    std::string buffer(std::istreambuf_iterator<char>(input_file), {});

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

void process_file(const std::string &input_path, const std::string &output_dir) {
    // Ищем файл маппинга
    std::unordered_map<std::basic_string<char>, std::basic_string<char>> file_mapping;
    std::streampos map_offset = find_file_map_offset(input_path);
    if (map_offset != -1) {
       file_mapping = read_file_mapping(input_path, map_offset);
    }
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

    int file_counter = 1;  // Добавляем переменную для отслеживания порядка файлов

    while (offset < file_size) {
        input_file.seekg(offset, std::ios::beg);
        Header header;
        input_file.read(reinterpret_cast<char *>(&header), sizeof(Header));

        if (header.magic1 != MD1IMG_MAGIC1 || header.magic2 != MD1IMG_MAGIC2) {
            break;
        }

        std::string name(header.name, strnlen(header.name, sizeof(header.name)));
        std::cout << "Found " << name << "@0x" << std::hex << header.base << ",0x" << header.data_size << std::dec << std::endl;

        // Добавляем порядковый номер к имени файла
        std::string output_name;
        if (file_mapping.find(name) != file_mapping.end()) {
            output_name = output_dir + "/" + std::to_string(file_counter) + "_" + file_mapping.at(name);
        }else{
            output_name = output_dir + "/" + std::to_string(file_counter) + "_" + name;
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
        file_counter += 1;
    }
}