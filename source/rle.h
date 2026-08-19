#ifndef RLE_H
#define RLE_H

#include <cstdint>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Run-Length Encoding (RLE)
//
// Dinh dang file nen: day cac cap 2 byte [count][value], khong co header.
//   - count: so lan lap, 1..255 (MAX_RUN)
//   - value: byte duoc lap
// Chuoi lap dai hon 255 duoc cat thanh nhieu cap lien tiep (xu ly tran so dem).
// Vi du: "AAAAABBBBBCCCCCDDDDD" (20 byte) -> 5A 5B 5C 5D (8 byte).
// ---------------------------------------------------------------------------

// So lan lap toi da ma mot cap [count][value] bieu dien duoc.
const int RLE_MAX_RUN = 255;

// Tang trong bo nho: dung cho unit test va do thoi gian thuan cua thuat toan.
std::vector<uint8_t> rle_encode(const std::vector<uint8_t>& data);
std::vector<uint8_t> rle_decode(const std::vector<uint8_t>& data);

// Tang file: giao dien chung ma CLI goi den.
void compress_rle(const std::string& input_path, const std::string& output_path);
void decompress_rle(const std::string& input_path, const std::string& output_path);

#endif // RLE_H
