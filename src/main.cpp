#include "main.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <pack|unpack> <input> [output_dir]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    std::string input_path = argv[2];
    std::filesystem::path output_dir;

    if (mode == "unpack") {
        if (argc > 3) {
            output_dir = argv[3];
        } else {
            std::filesystem::path input_file_path(input_path);
            output_dir = input_file_path.parent_path() / input_file_path.stem();
        }
        // Ищем файл маппинга
        std::streampos map_offset = find_file_map_offset(input_path);
        if (map_offset != -1) {
            auto file_mapping = read_file_mapping(input_path, map_offset);
            process_file(input_path, output_dir.string(), file_mapping);
            //unpack(input_path, output_dir.string());
        }
    } else if (mode == "pack") {
        std::filesystem::path input_dir_path(input_path);

        if (!std::filesystem::exists(input_dir_path) || !std::filesystem::is_directory(input_dir_path)) {
            std::cerr << "Error: input directory does not exist or is not a directory." << std::endl;
            return 1;
        }

        // Формируем имя выходного файла: <имя_папки>-new.img
        std::filesystem::path output_file_path = input_dir_path.parent_path() / (input_dir_path.stem().string() + "-new.img");

        // Пакуем файлы в новый образ
        write_packed_file(input_path, output_file_path.string());
    } else {
        std::cerr << "Invalid mode: " << mode << ". Use 'pack' or 'unpack'." << std::endl;
        return 1;
    }

    return 0;
}