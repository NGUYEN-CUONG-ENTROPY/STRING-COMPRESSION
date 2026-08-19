#ifndef LZW_H
#define LZW_H

#include <string>

void compress_lzw(const std::string& input_path, const std::string& output_path);
void decompress_lzw(const std::string& input_path, const std::string& output_path);

#endif // LZW_H