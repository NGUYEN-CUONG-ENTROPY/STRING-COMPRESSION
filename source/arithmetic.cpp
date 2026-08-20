#include "arithmetic.h"
#include "bit_io.h"
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

const uint32_t MAX_VALUE = 0xFFFFFFFF;
const uint32_t HALF      = 0x80000000;
const uint32_t QUARTER_1 = 0x40000000;

// Hàm tạo bảng phân phối tích lũy từ mảng tần suất
void build_model(const vector<uint32_t>& freqs, ArithmeticModel& model) {
    model.cumulative_freq.assign(ALPHABET_SIZE + 1, 0);
    uint32_t sum = 0;
    for (uint32_t i = 0; i < ALPHABET_SIZE; ++i) {
        model.cumulative_freq[i] = sum;
        sum += freqs[i];
    }
    model.cumulative_freq[ALPHABET_SIZE] = sum;
    model.total_count = sum;
}

bool compress_arithmetic(const string& input_file, const string& output_file) {
    ifstream in(input_file, ios::binary);
    if (!in) {
        cerr << "Error: Cannot open input file " << input_file << endl;
        return false;
    }

    // Lần quét 1 (Pass 1): Đếm tần suất
    vector<uint32_t> freqs(ALPHABET_SIZE, 0);
    char c;
    while (in.get(c)) {
        freqs[(unsigned char)c]++;
    }
    freqs[EOF_SYMBOL] = 1; // Đảm bảo EOF có tần suất ít nhất là 1
    
    // Đảm bảo không có ký tự nào bị freq = 0 mà lại xuất hiện (đã handle)
    // Tối ưu để không bị chia cho số quá nhỏ hoặc tràn
    ArithmeticModel model;
    build_model(freqs, model);

    in.clear();
    in.seekg(0, ios::beg);

    ofstream out(output_file, ios::binary);
    if (!out) {
        cerr << "Error: Cannot open output file " << output_file << endl;
        return false;
    }

    // Ghi Header: Ghi bảng tần suất (256 uint32_t)
    // Chú ý: Ta chỉ cần ghi 256 ký tự ASCII. EOF luôn là 1.
    for (int i = 0; i < 256; ++i) {
        uint32_t f = freqs[i];
        out.write(reinterpret_cast<const char*>(&f), sizeof(f));
    }

    BitWriter bit_writer(out);

    // Lần quét 2 (Pass 2): Mã hóa Arithmetic
    uint32_t low = 0;
    uint32_t high = MAX_VALUE;
    uint32_t pending_bits = 0;

    auto encode_symbol = [&](uint32_t symbol) {
        uint64_t range = (uint64_t)high - low + 1;
        high = low + (range * model.cumulative_freq[symbol + 1]) / model.total_count - 1;
        low = low + (range * model.cumulative_freq[symbol]) / model.total_count;

        // Renormalization
        while (true) {
            if ((high & HALF) == (low & HALF)) {
                bool bit = (high & HALF) != 0;
                bit_writer.writeBit(bit);
                while (pending_bits > 0) {
                    bit_writer.writeBit(!bit);
                    pending_bits--;
                }
            } else if ((low & QUARTER_1) && !(high & QUARTER_1)) {
                // Tình huống Underflow: low = 01... và high = 10...
                pending_bits++;
                low ^= QUARTER_1;
                high ^= QUARTER_1;
            } else {
                break;
            }
            low = (low << 1);
            high = (high << 1) | 1;
        }
    };

    while (in.get(c)) {
        encode_symbol((unsigned char)c);
    }
    
    // Đẩy EOF_SYMBOL
    encode_symbol(EOF_SYMBOL);

    // Đẩy các bit cuối (Flush)
    pending_bits++;
    bool bit = (low & QUARTER_1) != 0;
    bit_writer.writeBit(bit);
    while (pending_bits > 0) {
        bit_writer.writeBit(!bit);
        pending_bits--;
    }
    bit_writer.flush();

    return true;
}

bool decompress_arithmetic(const string& input_file, const string& output_file) {
    ifstream in(input_file, ios::binary);
    if (!in) {
        cerr << "Error: Cannot open input file " << input_file << endl;
        return false;
    }

    // Đọc Header: 256 uint32_t
    vector<uint32_t> freqs(ALPHABET_SIZE, 0);
    for (int i = 0; i < 256; ++i) {
        uint32_t f;
        if (!in.read(reinterpret_cast<char*>(&f), sizeof(f))) {
            return false; // Lỗi file rỗng hoặc hỏng
        }
        freqs[i] = f;
    }
    freqs[EOF_SYMBOL] = 1;

    ArithmeticModel model;
    build_model(freqs, model);

    BitReader bit_reader(in);
    ofstream out(output_file, ios::binary);
    if (!out) {
        cerr << "Error: Cannot open output file " << output_file << endl;
        return false;
    }

    uint32_t low = 0;
    uint32_t high = MAX_VALUE;
    uint32_t value = 0;

    // Đọc 32 bit đầu tiên vào value
    for (int i = 0; i < 32; ++i) {
        bool bit = false;
        bit_reader.readBit(bit);
        value = (value << 1) | (bit ? 1U : 0U);
    }

    while (true) {
        uint64_t range = (uint64_t)high - low + 1;
        uint64_t scaled_value = ((uint64_t)(value - low) + 1) * model.total_count - 1;
        scaled_value /= range;

        // Dò tìm symbol
        uint32_t symbol = ALPHABET_SIZE - 1; // Mặc định là phần tử cuối
        for (uint32_t i = 0; i < ALPHABET_SIZE; ++i) {
            if (model.cumulative_freq[i + 1] > scaled_value) {
                symbol = i;
                break;
            }
        }

        if (symbol == EOF_SYMBOL) break;
        out.put(static_cast<char>(symbol));

        // Cập nhật khoảng [low, high)
        high = low + (range * model.cumulative_freq[symbol + 1]) / model.total_count - 1;
        low = low + (range * model.cumulative_freq[symbol]) / model.total_count;

        // Renormalization
        while (true) {
            if ((high & HALF) == (low & HALF)) {
                // Không làm gì thêm, chỉ shift
            } else if ((low & QUARTER_1) && !(high & QUARTER_1)) {
                low ^= QUARTER_1;
                high ^= QUARTER_1;
                value ^= QUARTER_1;
            } else {
                break;
            }
            low = (low << 1);
            high = (high << 1) | 1;
            bool bit = false;
            bit_reader.readBit(bit);
            value = (value << 1) | (bit ? 1U : 0U);
        }
    }

    return true;
}
