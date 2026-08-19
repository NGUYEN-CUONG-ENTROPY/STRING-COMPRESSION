#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

// Dong ho do thoi gian chay cua thuat toan, don vi mili-giay (ms).
class Timer {
public:
    void start();
    double elapsed_ms() const;

private:
    std::chrono::high_resolution_clock::time_point start_time;
};

// Doc/ghi toan bo file o dang nhi phan. Nem std::runtime_error neu that bai.
std::vector<uint8_t> read_file(const std::string& path);
void write_file(const std::string& path, const std::vector<uint8_t>& data);

// Kich thuoc file tinh bang byte, -1 neu khong mo duoc.
long long get_file_size(const std::string& path);

// In bang tong ket hieu nang theo dinh dang yeu cau cua do an.
void print_compression_summary(const std::string& algo_name, double elapsed_ms,
                               long long original_size, long long compressed_size);
void print_decompression_summary(const std::string& algo_name, double elapsed_ms,
                                 long long compressed_size, long long restored_size);

#endif // UTILS_H
