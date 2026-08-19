#include "utils.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

void Timer::start() {
    start_time = std::chrono::high_resolution_clock::now();
}

double Timer::elapsed_ms() const {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> diff = now - start_time;
    return diff.count();
}

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream infile(path, std::ios::binary);
    if (!infile) throw std::runtime_error("Khong mo duoc file dau vao: " + path);

    infile.seekg(0, std::ios::end);
    std::streamoff size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(static_cast<size_t>(size));
    if (size > 0) {
        infile.read(reinterpret_cast<char*>(data.data()), size);
        if (!infile) throw std::runtime_error("Loi khi doc file: " + path);
    }
    return data;
}

void write_file(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream outfile(path, std::ios::binary);
    if (!outfile) throw std::runtime_error("Khong mo duoc file dau ra: " + path);

    if (!data.empty()) {
        outfile.write(reinterpret_cast<const char*>(data.data()),
                      static_cast<std::streamsize>(data.size()));
    }
    if (!outfile) throw std::runtime_error("Loi khi ghi file: " + path);
}

long long get_file_size(const std::string& path) {
    std::ifstream infile(path, std::ios::binary | std::ios::ate);
    if (!infile) return -1;
    return static_cast<long long>(infile.tellg());
}

// Duong ke phan cach trong bang tong ket.
static const char* kSeparator = "--------------------------------";

void print_compression_summary(const std::string& algo_name, double elapsed_ms,
                               long long original_size, long long compressed_size) {
    std::cout << "Compression complete.\n";
    std::cout << kSeparator << "\n";
    std::cout << "Algorithm: " << algo_name << "\n";
    std::cout << "Execution Time: " << std::fixed << std::setprecision(2) << elapsed_ms << " ms\n";
    std::cout << "Original Size: " << original_size << " bytes\n";
    std::cout << "Compressed Size: " << compressed_size << " bytes\n";

    // Ti so nen = goc / nen; tiet kiem = 1 - nen / goc. Tranh chia cho 0 voi file rong.
    if (compressed_size > 0 && original_size > 0) {
        double ratio = static_cast<double>(original_size) / static_cast<double>(compressed_size);
        double savings = 1.0 - static_cast<double>(compressed_size) / static_cast<double>(original_size);
        std::cout << "Compression Ratio: " << std::fixed << std::setprecision(2) << ratio << "\n";
        std::cout << "Space Savings: " << std::fixed << std::setprecision(1) << savings * 100.0 << "%\n";
    } else {
        std::cout << "Compression Ratio: N/A\n";
        std::cout << "Space Savings: N/A\n";
    }
}

void print_decompression_summary(const std::string& algo_name, double elapsed_ms,
                                 long long compressed_size, long long restored_size) {
    std::cout << "Decompression complete.\n";
    std::cout << kSeparator << "\n";
    std::cout << "Algorithm: " << algo_name << "\n";
    std::cout << "Execution Time: " << std::fixed << std::setprecision(2) << elapsed_ms << " ms\n";
    std::cout << "Compressed Size: " << compressed_size << " bytes\n";
    std::cout << "Decompressed Size: " << restored_size << " bytes\n";
}
