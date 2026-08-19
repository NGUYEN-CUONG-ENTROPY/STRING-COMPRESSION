#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include <string>
#include <vector>
#include <cstdint>

// Hằng số đánh dấu ký tự kết thúc file (End-of-File).
// Bảng mã ASCII có 256 ký tự (0-255), nên ta dùng 256 làm EOF_SYMBOL.
const uint32_t EOF_SYMBOL = 256;
const uint32_t ALPHABET_SIZE = 257; // 256 ký tự ASCII + 1 EOF_SYMBOL

// Cấu trúc bảng phân phối tích lũy để dùng chung cho cả nén và giải nén
struct ArithmeticModel {
    std::vector<uint32_t> cumulative_freq;
    uint32_t total_count;

    ArithmeticModel() {
        cumulative_freq.assign(ALPHABET_SIZE + 1, 0);
        total_count = 0;
    }
};

// Hàm nén file
// Trả về true nếu thành công, false nếu gặp lỗi (như không mở được file)
bool compress_arithmetic(const std::string& input_file, const std::string& output_file);

// Hàm giải nén file
// Trả về true nếu thành công, false nếu gặp lỗi
bool decompress_arithmetic(const std::string& input_file, const std::string& output_file);

#endif // ARITHMETIC_H
