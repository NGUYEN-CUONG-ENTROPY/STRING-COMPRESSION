#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <string>

void compress_huffman(const std::string& input_path, const std::string& output_path);
void decompress_huffman(const std::string& input_path, const std::string& output_path);

#endif // HUFFMAN_H
