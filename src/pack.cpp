#include "main.h"

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
