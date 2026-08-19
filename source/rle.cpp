#include "rle.h"

#include <stdexcept>

#include "utils.h"

// Nen: duyet mot lan, dem don so byte giong nhau lien tiep.
// Khi dat nguong RLE_MAX_RUN thi chot mot cap va bat dau dem lai tu dau
// -> chuoi lap dai bao nhieu cung khong lam tran o dem 1 byte.
std::vector<uint8_t> rle_encode(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out;
    // Truong hop xau nhat (khong co byte nao lap) can gap doi kich thuoc.
    out.reserve(data.size() * 2);

    size_t i = 0;
    while (i < data.size()) {
        uint8_t value = data[i];
        size_t run = 1;
        while (i + run < data.size() && data[i + run] == value &&
               run < static_cast<size_t>(RLE_MAX_RUN)) {
            ++run;
        }
        out.push_back(static_cast<uint8_t>(run));
        out.push_back(value);
        i += run;
    }
    return out;
}

// Giai nen: doc tung cap [count][value] va bung lai count ban sao cua value.
std::vector<uint8_t> rle_decode(const std::vector<uint8_t>& data) {
    if (data.size() % 2 != 0) {
        throw std::runtime_error("RLE Error: file nen bi hong (so byte phai la so chan).");
    }

    // Duyet truoc de biet chinh xac kich thuoc ket qua, tranh cap phat lai nhieu lan.
    size_t total = 0;
    for (size_t i = 0; i < data.size(); i += 2) {
        if (data[i] == 0) {
            throw std::runtime_error("RLE Error: file nen bi hong (so lan lap bang 0).");
        }
        total += data[i];
    }

    std::vector<uint8_t> out;
    out.reserve(total);
    for (size_t i = 0; i < data.size(); i += 2) {
        out.insert(out.end(), data[i], data[i + 1]);
    }
    return out;
}

void compress_rle(const std::string& input_path, const std::string& output_path) {
    std::vector<uint8_t> input = read_file(input_path);
    std::vector<uint8_t> encoded = rle_encode(input);
    write_file(output_path, encoded);
}

void decompress_rle(const std::string& input_path, const std::string& output_path) {
    std::vector<uint8_t> input = read_file(input_path);
    std::vector<uint8_t> decoded = rle_decode(input);
    write_file(output_path, decoded);
}
