#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <string>

void compress_huffman(const std::string &inputPath, const std::string &outputPath);
void decompress_huffman(const std::string &inputPath, const std::string &outputPath);

#endif
