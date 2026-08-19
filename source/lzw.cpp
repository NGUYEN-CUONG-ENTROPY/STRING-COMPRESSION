#include "lzw.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <stdexcept>

void compress_lzw(const std::string& input_path, const std::string& output_path) {
    std::ifstream infile(input_path, std::ios::binary);
    if (!infile) throw std::runtime_error("LZW Error: Cannot open input file.");

    std::vector<uint8_t> input_data((std::istreambuf_iterator<char>(infile)), 
                                     std::istreambuf_iterator<char>());
    infile.close();

    std::unordered_map<std::string, uint16_t> dictionary;
    for (uint16_t i = 0; i <= 255; i++) {
        dictionary[std::string(1, static_cast<char>(i))] = i;
    }
    uint16_t dictSize = 256;
    
    std::string w = "";
    std::vector<uint16_t> compressed_data;

    for (uint8_t c : input_data) {
        std::string wc = w + static_cast<char>(c);
        if (dictionary.count(wc)) {
            w = wc;
        } else {
            compressed_data.push_back(dictionary[w]);
            if (dictSize < 65535) { 
                dictionary[wc] = dictSize++;
            }
            w = std::string(1, static_cast<char>(c));
        }
    }
    if (!w.empty()) {
        compressed_data.push_back(dictionary[w]);
    }

    std::ofstream outfile(output_path, std::ios::binary);
    if (!outfile) throw std::runtime_error("LZW Error: Cannot open output file.");
    
    for (uint16_t code : compressed_data) {
        outfile.write(reinterpret_cast<const char*>(&code), sizeof(code));
    }
    outfile.close();
}

void decompress_lzw(const std::string& input_path, const std::string& output_path) {
    std::ifstream infile(input_path, std::ios::binary);
    if (!infile) throw std::runtime_error("LZW Error: Cannot open compressed file.");

    std::vector<uint16_t> compressed_data;
    uint16_t code;
    while (infile.read(reinterpret_cast<char*>(&code), sizeof(code))) {
        compressed_data.push_back(code);
    }
    infile.close();

    if (compressed_data.empty()) {
        std::ofstream outfile(output_path, std::ios::binary);
        return; // File rỗng
    }

    std::unordered_map<uint16_t, std::string> dictionary;
    for (uint16_t i = 0; i <= 255; i++) {
        dictionary[i] = std::string(1, static_cast<char>(i));
    }
    uint16_t dictSize = 256;

    std::string w = dictionary[compressed_data[0]];
    std::string decompressed_data = w;

    for (size_t i = 1; i < compressed_data.size(); i++) {
        uint16_t k = compressed_data[i];
        std::string entry;
        
        if (dictionary.count(k)) {
            entry = dictionary[k];
        } else if (k == dictSize) {
            entry = w + w[0];
        } else {
            throw std::runtime_error("LZW Error: Invalid compressed code encountered.");
        }
        
        decompressed_data += entry;
        
        if (dictSize < 65535) {
            dictionary[dictSize++] = w + entry[0];
        }
        w = entry;
    }

    std::ofstream outfile(output_path, std::ios::binary);
    if (!outfile) throw std::runtime_error("LZW Error: Cannot open output file.");
    outfile.write(decompressed_data.c_str(), decompressed_data.size());
    outfile.close();
}