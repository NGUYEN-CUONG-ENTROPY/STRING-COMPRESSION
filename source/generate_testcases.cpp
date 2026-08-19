#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

const size_t KB = 1024;
const size_t MB = 1024 * KB;

// Hàm hỗ trợ ghi file với chuỗi lặp lại
void generate_repetitive_file(const std::string& filepath, const std::string& pattern, size_t target_size) {
    std::ofstream outfile(filepath, std::ios::binary);
    if (!outfile) {
        std::cerr << "Khong the tao file: " << filepath << "\n";
        return;
    }

    size_t written = 0;
    while (written < target_size) {
        size_t to_write = std::min(pattern.length(), target_size - written);
        outfile.write(pattern.c_str(), to_write);
        written += to_write;
    }
    outfile.close();
    std::cout << "Da tao: " << filepath << " (" << target_size << " bytes)\n";
}

// Hàm hỗ trợ ghi file với chuỗi ngẫu nhiên
void generate_random_file(const std::string& filepath, size_t target_size) {
    std::ofstream outfile(filepath, std::ios::binary);
    if (!outfile) {
        std::cerr << "Khong the tao file: " << filepath << "\n";
        return;
    }

    // Thiết lập bộ sinh số ngẫu nhiên cho các ký tự ASCII in được (từ 32 đến 126)
    std::mt19937 gen(42); // Cố định seed để các thành viên trong nhóm ra file giống nhau
    std::uniform_int_distribution<> dis(32, 126);

    const size_t buffer_size = 4096;
    char buffer[buffer_size];
    size_t written = 0;

    while (written < target_size) {
        size_t to_write = std::min(buffer_size, target_size - written);
        for (size_t i = 0; i < to_write; ++i) {
            buffer[i] = static_cast<char>(dis(gen));
        }
        outfile.write(buffer, to_write);
        written += to_write;
    }
    outfile.close();
    std::cout << "Da tao: " << filepath << " (" << target_size << " bytes)\n";
}

int main() {
    std::string test_dir = "tests";

    // Tạo thư mục tests nếu chưa có
    if (!fs::exists(test_dir)) {
        fs::create_directory(test_dir);
    }

    // ==========================================
    // SCENARIO 1: Impact of File Size[cite: 1]
    // ==========================================
    std::string english_text = "Data compression is a fundamental concept in computer science. It allows us to efficiently manage limited storage and transmit data rapidly over networks. ";
    
    generate_repetitive_file(test_dir + "/english_10kb.txt", english_text, 10 * KB);
    generate_repetitive_file(test_dir + "/english_100kb.txt", english_text, 100 * KB);
    generate_repetitive_file(test_dir + "/english_1mb.txt", english_text, 1 * MB);
    generate_repetitive_file(test_dir + "/english_10mb.txt", english_text, 10 * MB);

    // ==========================================
    // SCENARIO 2: Impact of Data Entropy[cite: 1]
    // ==========================================
    // 1. Highly repetitive text
    std::string highly_repetitive = "AAAAABBBBB";
    generate_repetitive_file(test_dir + "/repetitive_1mb.txt", highly_repetitive, 1 * MB);

    // 2. Standard English prose (Đã tạo ở Scenario 1 là english_1mb.txt)

    // 3. Completely randomized character strings
    generate_random_file(test_dir + "/random_1mb.txt", 1 * MB);

    std::cout << "\nHoan tat tao toan bo testcase trong thu muc '" << test_dir << "'!\n";

    return 0;
}