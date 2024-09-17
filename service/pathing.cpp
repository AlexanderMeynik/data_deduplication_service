#include <filesystem>
#include <fstream>
#include <iostream>
#include <glog/logging.h>
namespace fs = std::filesystem;

void create_and_write_file(const std::string& root_dir, const std::string& relative_path, const std::string& file_contents) {
    // Combine the root directory with the relative path
    fs::path full_path = fs::path(root_dir) / relative_path;

    // Get the parent directory (i.e., the directory containing the file)
    fs::path parent_dir = full_path.parent_path();

    // Check if the parent directory exists, if not, create it
    if (!fs::exists(parent_dir)) {
        std::cout << "Creating directories: " << parent_dir << '\n';
        fs::create_directories(parent_dir);
    }

    // Open the file and write the contents
    std::ofstream file(full_path);
    if (file.is_open()) {
        file << file_contents;
        file.close();
        std::cout << "File created and contents written to: " << full_path << '\n';
    } else {
        std::cerr << "Failed to create file: " << full_path << '\n';
    }
}

int main() {
    std::string root_dir = "/mnt/c/projects/data_deduplication_service/train";
    std::string relative_path = "service/res1/config.txt";
    std::string file_contents = "This is the content of config.txt";

    // Call the function to create directories and write the file
    create_and_write_file(root_dir, relative_path, file_contents);

    return 0;
}