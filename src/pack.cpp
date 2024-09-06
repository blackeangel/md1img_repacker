#include "main.h"
/*
void write_packed_file(const std::string& input_dir, const std::string& output_file_path) {
    std::ofstream output_file(output_file_path, std::ios::binary);
    if (!output_file) {
        std::cerr << "Error creating output file: " << output_file_path << std::endl;
        return;
    }

    uint32_t offset = 0;
    std::vector<std::filesystem::path> files;

    // Собираем все бинарные файлы из папки
    for (const auto& entry : std::filesystem::directory_iterator(input_dir)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }

    for (const auto& file_path : files) {
        std::ifstream input_file(file_path, std::ios::binary | std::ios::ate);
        if (!input_file) {
            std::cerr << "Error opening input file: " << file_path << std::endl;
            continue;
        }

        // Получаем размер данных
        std::streamsize data_size = input_file.tellg();
        input_file.seekg(0, std::ios::beg);

        // Читаем данные файла
        std::vector<char> data(data_size);
        input_file.read(data.data(), data_size);

        // Формируем имя файла без пути
        std::string file_name = file_path.stem().string();

        // Создаем заголовок
        Header header = {};
        header.magic1 = MD1IMG_MAGIC1;
        header.data_size = static_cast<uint32_t>(data_size);
        std::strncpy(header.name, file_name.c_str(), sizeof(header.name) - 1);
        header.base = 0; // Значение можно поменять, если известно
        header.mode = 0; // Тоже можно изменить при необходимости
        header.magic2 = MD1IMG_MAGIC2;
        header.data_offset = offset + sizeof(Header);

        // Записываем заголовок и данные в выходной файл
        output_file.write(reinterpret_cast<const char*>(&header), sizeof(Header));
        output_file.write(data.data(), data_size);

        std::cout << "Packed " << file_name << " into " << output_file_path << std::endl;

        // Обновляем offset (выравнивание на 16 байт)
        offset = header.data_offset + data_size;
        if (offset % 16 != 0) {
            offset += 16 - (offset % 16);
        }
    }

    std::cout << "Packing completed: " << output_file_path << std::endl;
}
*/

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
            std::string key = line.substr(0, delimiter_pos);
            std::string value = line.substr(delimiter_pos + 1);
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

    std::memset(header.reserved, 0, sizeof(header.reserved));
    std::memset(header.reserved2, 0xFF, sizeof(header.reserved2));
}

// Компаратор для сортировки файлов
bool file_sort_comparator(const std::filesystem::path &file1, const std::filesystem::path &file2) {
    int base1 = extract_number(file1.filename().string());
    int base2 = extract_number(file2.filename().string());
    return base1 < base2;
}

// Основная функция для упаковки файлов
void pack_files(const std::string &input_dir, const std::string &output_file) {
    std::vector<std::filesystem::path> files;
    std::filesystem::path map_file_path;
    bool map_file_found = false;

    // Сначала ищем файл с маппингом (md1_file_map)
    for (const auto &entry : std::filesystem::directory_iterator(input_dir)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.find("md1_file_map") != std::string::npos) {  // Проверяем, содержит ли имя файла "md1_file_map"
                map_file_path = entry.path();
                map_file_found = true;
                break;
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

    // Собираем список файлов для упаковки
    for (const auto &entry : std::filesystem::directory_iterator(input_dir)) {
        //if (entry.is_regular_file() && entry.path() != map_file_path) {  // Пропускаем файл маппинга
            files.push_back(entry.path());
        //}
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
        initialize_header(header, mapped_name, file_data.size(), base_address);

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

    /*// Добавляем md1_file_map в конец файла
    std::string file_map = "md1_file_map=" + output_file;
    output.write(file_map.c_str(), file_map.size());
    */
    std::cout << "Packing complete: " << output_file << std::endl;
}