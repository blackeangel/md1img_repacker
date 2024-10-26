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

    input_file.seekg(map_offset + static_cast<std::streamoff>(504)); //504 bytes - остатки от header md1_file_ma, которые надо пропустить, иначе не всё переводится в нормальные файлы
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

/*
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

    // Открываем файл для записи meta_info
    std::ofstream meta_info_file(output_dir + "/meta_info");
    if (!meta_info_file) {
        std::cerr << "Error opening meta_info file for writing" << std::endl;
        return;
    }

    while (offset < file_size) {
        input_file.seekg(offset, std::ios::beg);
        Header header{};
        input_file.read(reinterpret_cast<char *>(&header), sizeof(Header));

        if (header.magic1 != MD1IMG_MAGIC1 || header.magic2 != MD1IMG_MAGIC2) {
            break;
        }

        std::string name(header.name, strnlen(header.name, sizeof(header.name)));
        std::cout << "Found " << name << "@0x" << std::hex << header.base << ",0x" << header.data_size << std::dec << std::endl;

        // Запись заголовка в meta_info
        meta_info_file << "name=" << name << "\n";
        meta_info_file << "magic1=0x" << std::hex << header.magic1 << std::dec << "\n";
        meta_info_file << "data_size=" << header.data_size << "\n";
        meta_info_file << "base=0x" << std::hex << header.base << std::dec << "\n";
        meta_info_file << "mode=0x" << std::hex << header.mode << std::dec << "\n";
        meta_info_file << "magic2=0x" << std::hex << header.magic2 << std::dec << "\n";
        meta_info_file << "data_offset=0x" << std::hex << header.data_offset << std::dec << "\n";
        meta_info_file << "hdr_version=0x" << std::hex << header.hdr_version << std::dec << "\n";
        meta_info_file << "img_type=0x" << std::hex << header.img_type << std::dec << "\n";
        meta_info_file << "img_list_end=0x" << std::hex << header.img_list_end << std::dec << "\n";
        meta_info_file << "align_size=0x" << std::hex << header.align_size << std::dec << "\n";
        meta_info_file << "dsize_extend=0x" << std::hex << header.dsize_extend << std::dec << "\n";
        meta_info_file << "maddr_extend=0x" << std::hex << header.maddr_extend << std::dec << "\n\n";

        std::string output_name;
        if (file_mapping.find(name) != file_mapping.end()) {
            output_name = output_dir + "/" + std::to_string(file_counter) + "_" + file_mapping.at(name);
        } else {
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

    // Закрываем файл meta_info
    meta_info_file.close();
    std::cout << "meta_info file written to " << output_dir << "/meta_info" << std::endl;
}
*/
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

    // Открываем файл для записи meta_info
    std::ofstream meta_info_file(output_dir + "/meta_info");
    if (!meta_info_file) {
        std::cerr << "Error opening meta_info file for writing" << std::endl;
        return;
    }

    while (offset < file_size) {
        input_file.seekg(offset, std::ios::beg);
        Header header{};
        input_file.read(reinterpret_cast<char *>(&header), sizeof(Header));

        if (header.magic1 != MD1IMG_MAGIC1 || header.magic2 != MD1IMG_MAGIC2) {
            break;
        }

        std::string name(header.name, strnlen(header.name, sizeof(header.name)));
        std::cout << "Found " << name << "@0x" << std::hex << header.base << ",0x" << header.data_size << std::dec << std::endl;

        // Запись заголовка в meta_info
        meta_info_file << "name=" << name << "\n";
        meta_info_file << "magic1=0x" << std::hex << header.magic1 << std::dec << "\n";
        meta_info_file << "data_size=" << header.data_size << "\n";
        meta_info_file << "base=0x" << std::hex << header.base << std::dec << "\n";
        meta_info_file << "mode=0x" << std::hex << header.mode << std::dec << "\n";
        meta_info_file << "magic2=0x" << std::hex << header.magic2 << std::dec << "\n";
        meta_info_file << "data_offset=0x" << std::hex << header.data_offset << std::dec << "\n";
        meta_info_file << "hdr_version=0x" << std::hex << header.hdr_version << std::dec << "\n";
        meta_info_file << "img_type=0x" << std::hex << header.img_type << std::dec << "\n";
        meta_info_file << "img_list_end=0x" << std::hex << header.img_list_end << std::dec << "\n";
        meta_info_file << "align_size=0x" << std::hex << header.align_size << std::dec << "\n";
        meta_info_file << "dsize_extend=0x" << std::hex << header.dsize_extend << std::dec << "\n";
        meta_info_file << "maddr_extend=0x" << std::hex << header.maddr_extend << std::dec << "\n\n";

        std::string output_name;
        if (file_mapping.find(name) != file_mapping.end()) {
            output_name = output_dir + "/" + std::to_string(file_counter) + "_" + file_mapping.at(name);
        } else {
            output_name = output_dir + "/" + std::to_string(file_counter) + "_" + name;
        }

        offset += header.data_offset;
        input_file.seekg(offset, std::ios::beg);

        std::vector<char> data(header.data_size);
        input_file.read(data.data(), header.data_size);

        // Проверка формата файла для распаковки
        std::string output_name1 = to_lowercase(output_name);
        if (ends_with(output_name1, ".gz") && is_gz_format(data)) {
            output_name = output_name.substr(0, output_name.size() - 3);  // Убираем ".gz" из имени
            auto decompressed_data = decompress_gz(data);
            std::ofstream(output_name, std::ios::binary).write(decompressed_data.data(), decompressed_data.size());
            std::cout << "Decompressed " << name << " to " << output_name << std::endl;
        } else if (ends_with(output_name1, ".xz") && is_xz_format(data)) {
            output_name = output_name.substr(0, output_name.size() - 3);  // Убираем ".xz" из имени
            auto decompressed_data = decompress_xz(data);
            std::ofstream(output_name, std::ios::binary).write(decompressed_data.data(), decompressed_data.size());
            std::cout << "Decompressed " << name << " to " << output_name << std::endl;
        } else {
            // Если распаковка не требуется, сохраняем файл как есть
            std::ofstream output_file(output_name, std::ios::binary);
            if (!output_file) {
                std::cerr << "Error opening output file for writing: " << output_name << std::endl;
                return;
            }
            output_file.write(data.data(), header.data_size);
            std::cout << name << " written to " << output_name << std::endl;
        }

        offset += header.data_size;
        if (offset % 16 != 0) {
            offset += 16 - (offset % 16);
        }
        file_counter += 1;
    }

    // Закрываем файл meta_info
    meta_info_file.close();
    std::cout << "meta_info file written to " << output_dir << "/meta_info" << std::endl;
}
