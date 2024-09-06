#include "main.h"

// Функция для извлечения числового префикса из имени файла
int extract_number(const std::string &filename) {
    size_t pos = 0;
    int number = 0;

    // Извлекаем цифры из начала строки
    while (pos < filename.size() && std::isdigit(filename[pos])) {
        number = number * 10 + (filename[pos] - '0');
        ++pos;
    }
    return number;
}

// Функция для удаления порядкового номера из имени файла
std::string strip_number(const std::string &filename) {
    size_t pos = 0;
    while (pos < filename.size() && std::isdigit(filename[pos])) {
        ++pos;
    }
    if (pos < filename.size() && filename[pos] == '_') {
        return filename.substr(pos + 1);
    }
    return filename;
}

// Функция для чтения маппинга файлов
std::unordered_map<std::string, std::string> read_file_mapping(const std::string &path) {
    std::unordered_map<std::string, std::string> mapping;
    std::ifstream map_file(path);

    if (!map_file) {
        std::cerr << "Error opening map file: " << path << std::endl;
        return mapping;
    }

    std::string line;
    while (std::getline(map_file, line)) {
        auto delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos) {
            //std::string key = line.substr(0, delimiter_pos);
            //std::string value = line.substr(delimiter_pos + 1);
            std::string value = line.substr(0, delimiter_pos);
            std::string key = line.substr(delimiter_pos + 1);
            mapping[key] = value;
        }
    }

    return mapping;
}

// Функция для инициализации заголовка
void initialize_header(Header &header, const std::string &name, uint32_t data_size, uint32_t base) {
    header.magic1 = MD1IMG_MAGIC1;
    header.data_size = data_size;
    std::strncpy(header.name, name.c_str(), sizeof(header.name));
    header.base = base;
    header.mode = 0;
    header.magic2 = MD1IMG_MAGIC2;
    header.data_offset = sizeof(Header);

    //std::memset(header.reserved, 0, sizeof(header.reserved));
    std::memset(header.reserved, 0xFF, sizeof(header.reserved));
}

// Компаратор для сортировки файлов
bool file_sort_comparator(const std::filesystem::path &file1, const std::filesystem::path &file2) {
    int base1 = extract_number(file1.filename().string());
    int base2 = extract_number(file2.filename().string());
    return base1 < base2;
}

// Функция для инициализации заголовка из файла meta_info
void initialize_header_from_file(Header &header, const std::unordered_map<std::string, std::string> &meta_data) {
    header.magic1 = std::stoul(meta_data.at("magic1"), nullptr, 16);
    header.data_size = std::stoul(meta_data.at("data_size"));
    std::strncpy(header.name, meta_data.at("name").c_str(), sizeof(header.name));
    header.base = std::stoul(meta_data.at("base"), nullptr, 16);
    header.mode = std::stoul(meta_data.at("mode"), nullptr, 16);
    header.magic2 = std::stoul(meta_data.at("magic2"), nullptr, 16);
    header.data_offset = std::stoul(meta_data.at("data_offset"), nullptr, 16);
    header.hdr_version = std::stoul(meta_data.at("hdr_version"), nullptr, 16);
    header.img_type = std::stoul(meta_data.at("img_type"), nullptr, 16);
    header.img_list_end = std::stoul(meta_data.at("img_list_end"), nullptr, 16);
    header.align_size = std::stoul(meta_data.at("align_size"), nullptr, 16);
    header.dsize_extend = std::stoul(meta_data.at("dsize_extend"), nullptr, 16);
    header.maddr_extend = std::stoul(meta_data.at("maddr_extend"), nullptr, 16);
    std::memset(header.reserved, 0xFF, sizeof(header.reserved));  // Как в распаковщике
}

// Функция для чтения данных из файла meta_info
std::unordered_map<std::string, std::string> read_meta_info(const std::string &meta_info_path) {
    std::unordered_map<std::string, std::string> meta_data;
    std::ifstream meta_info_file(meta_info_path);

    if (!meta_info_file) {
        std::cerr << "Error opening meta_info file: " << meta_info_path << std::endl;
        return meta_data;
    }

    std::string line;
    while (std::getline(meta_info_file, line)) {
        auto delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos) {
            std::string key = line.substr(0, delimiter_pos);
            std::string value = line.substr(delimiter_pos + 1);
            meta_data[key] = value;
        }
    }

    return meta_data;
}

// Основная функция для упаковки файлов с поддержкой meta_info
void pack_files(const std::string &input_dir, const std::string &output_file) {
    std::vector<std::filesystem::path> files;
    std::filesystem::path map_file_path;
    bool map_file_found = false;
    std::filesystem::path meta_info_path;
    bool meta_info_found = false;

    // Сначала ищем файл с маппингом (md1_file_map) и meta_info
    for (const auto &entry : std::filesystem::directory_iterator(input_dir)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.find("md1_file_map") != std::string::npos) {
                map_file_path = entry.path();
                map_file_found = true;
            } else if (filename == "meta_info") {
                meta_info_path = entry.path();
                meta_info_found = true;
            }
        }
    }

    // Если файл с маппингом не найден, возвращаем ошибку
    if (!map_file_found) {
        std::cerr << "Error: md1_file_map not found in directory: " << input_dir << std::endl;
        return;
    }

    // Чтение маппинга из файла
    std::unordered_map<std::string, std::string> file_mapping = read_file_mapping(map_file_path.string());

    // Чтение данных из meta_info, если он найден
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> meta_info_data;
    if (meta_info_found) {
        std::ifstream meta_file(meta_info_path);
        std::string line;
        std::unordered_map<std::string, std::string> current_meta_data;
        std::string current_file;

        // Читаем данные по каждому файлу
        while (std::getline(meta_file, line)) {
            if (line.find("name=") == 0) {
                if (!current_file.empty()) {
                    meta_info_data[current_file] = current_meta_data;
                }
                current_file = line.substr(5);  // Убираем "name="
                current_meta_data.clear();
            }
            auto delimiter_pos = line.find('=');
            if (delimiter_pos != std::string::npos) {
                std::string key = line.substr(0, delimiter_pos);
                std::string value = line.substr(delimiter_pos + 1);
                current_meta_data[key] = value;
            }
        }
        if (!current_file.empty()) {
            meta_info_data[current_file] = current_meta_data;
        }
    }

    // Собираем список файлов для упаковки
    for (const auto &entry : std::filesystem::directory_iterator(input_dir)) {
        if (entry.is_regular_file() && entry.path().filename().string() != "meta_info") {
            files.push_back(entry.path());
        }
    }

    // Сортируем файлы по именам
    std::sort(files.begin(), files.end(), file_sort_comparator);

    // Открываем выходной файл
    std::ofstream output(output_file, std::ios::binary);
    if (!output) {
        std::cerr << "Error opening output file: " << output_file << std::endl;
        return;
    }

    uint32_t base_address = 0;

    // Обрабатываем файлы
    for (const auto &file : files) {
        std::ifstream input(file, std::ios::binary);
        if (!input) {
            std::cerr << "Error opening input file: " << file << std::endl;
            continue;
        }

        // Читаем содержимое файла
        std::vector<char> file_data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

        // Извлекаем имя файла без порядкового номера
        std::string stripped_name = strip_number(file.filename().string());

        // Ищем соответствие в маппинге
        std::string mapped_name = stripped_name;
        if (file_mapping.find(stripped_name) != file_mapping.end()) {
            mapped_name = file_mapping.at(stripped_name);
        }

        // Инициализируем заголовок
        Header header;
        if (meta_info_found && meta_info_data.find(mapped_name) != meta_info_data.end()) {
            initialize_header_from_file(header, meta_info_data[mapped_name]);
        } else {
            initialize_header(header, mapped_name, file_data.size(), base_address);
        }

        // Записываем заголовок и данные в выходной файл
        output.write(reinterpret_cast<const char *>(&header), sizeof(Header));
        output.write(file_data.data(), file_data.size());

        std::cout << "Packed: " << file.filename().string() << " as " << mapped_name << " (Size: " << file_data.size() << " bytes)" << std::endl;

        // Выравнивание по 16 байт
        uint32_t padding = 16 - (file_data.size() % 16);
        if (padding != 16) {
            std::vector<char> padding_data(padding, 0);
            output.write(padding_data.data(), padding_data.size());
        }

        // Обновляем base_address
        base_address += file_data.size() + sizeof(Header) + padding;
    }

    std::cout << "Packing complete: " << output_file << std::endl;
}
