#include "huffman.h"

#include "bit_io.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <limits>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr std::array<char, 4> MAGIC = {'H', 'U', 'F', '1'};

struct Node {
    std::uint64_t frequency;
    int symbol;
    int minimumSymbol;
    Node* left;
    Node* right;

    bool isLeaf() const {
        return left == nullptr && right == nullptr;
    }
};

struct NodeGreater {
    bool operator()(const Node* first, const Node* second) const {
        if (first->frequency != second->frequency) {
            return first->frequency > second->frequency;
        }
        return first->minimumSymbol > second->minimumSymbol;
    }
};

using Frequencies = std::array<std::uint64_t, 256>;
using CodeTable = std::array<std::vector<bool>, 256>;

void writeUint16(std::ostream& output, std::uint16_t value) {
    for (int shift = 0; shift < 16; shift += 8) {
        output.put(static_cast<char>((value >> shift) & 0xFFU));
    }
    if (!output) {
        throw std::runtime_error("Huffman error: failed to write file header.");
    }
}

void writeUint64(std::ostream& output, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) {
        output.put(static_cast<char>((value >> shift) & 0xFFU));
    }
    if (!output) {
        throw std::runtime_error("Huffman error: failed to write file header.");
    }
}

std::uint16_t readUint16(std::istream& input) {
    std::uint16_t value = 0;
    for (int shift = 0; shift < 16; shift += 8) {
        char byte = 0;
        if (!input.get(byte)) {
            throw std::runtime_error("Huffman error: truncated file header.");
        }
        value |= static_cast<std::uint16_t>(static_cast<unsigned char>(byte)) << shift;
    }
    return value;
}

std::uint64_t readUint64(std::istream& input) {
    std::uint64_t value = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        char byte = 0;
        if (!input.get(byte)) {
            throw std::runtime_error("Huffman error: truncated file header.");
        }
        value |= static_cast<std::uint64_t>(static_cast<unsigned char>(byte)) << shift;
    }
    return value;
}

Node* buildTree(const Frequencies& frequencies,
                std::vector<std::unique_ptr<Node>>& nodes) {
    std::priority_queue<Node*, std::vector<Node*>, NodeGreater> queue;

    for (int symbol = 0; symbol < 256; ++symbol) {
        if (frequencies[static_cast<std::size_t>(symbol)] == 0) {
            continue;
        }
        nodes.push_back(std::make_unique<Node>(Node{
            frequencies[static_cast<std::size_t>(symbol)], symbol, symbol, nullptr, nullptr
        }));
        queue.push(nodes.back().get());
    }

    while (queue.size() > 1) {
        Node* left = queue.top();
        queue.pop();
        Node* right = queue.top();
        queue.pop();

        if (left->frequency > std::numeric_limits<std::uint64_t>::max() - right->frequency) {
            throw std::runtime_error("Huffman error: frequency total is too large.");
        }

        nodes.push_back(std::make_unique<Node>(Node{
            left->frequency + right->frequency,
            -1,
            std::min(left->minimumSymbol, right->minimumSymbol),
            left,
            right
        }));
        queue.push(nodes.back().get());
    }

    return queue.empty() ? nullptr : queue.top();
}

void buildCodes(const Node* node, std::vector<bool>& path, CodeTable& codes) {
    if (node->isLeaf()) {
        codes[static_cast<std::size_t>(node->symbol)] = path;
        return;
    }

    path.push_back(false);
    buildCodes(node->left, path, codes);
    path.back() = true;
    buildCodes(node->right, path, codes);
    path.pop_back();
}

void writeHeader(std::ostream& output,
                 std::uint64_t originalSize,
                 const Frequencies& frequencies) {
    output.write(MAGIC.data(), static_cast<std::streamsize>(MAGIC.size()));
    writeUint64(output, originalSize);

    std::uint16_t symbolCount = 0;
    for (std::uint64_t frequency : frequencies) {
        if (frequency != 0) {
            ++symbolCount;
        }
    }
    writeUint16(output, symbolCount);

    for (std::size_t symbol = 0; symbol < frequencies.size(); ++symbol) {
        if (frequencies[symbol] == 0) {
            continue;
        }
        output.put(static_cast<char>(symbol));
        writeUint64(output, frequencies[symbol]);
    }
}

std::uint64_t readHeader(std::istream& input, Frequencies& frequencies) {
    std::array<char, 4> magic{};
    if (!input.read(magic.data(), static_cast<std::streamsize>(magic.size())) || magic != MAGIC) {
        throw std::runtime_error("Huffman error: invalid file signature.");
    }

    const std::uint64_t originalSize = readUint64(input);
    const std::uint16_t symbolCount = readUint16(input);
    if (symbolCount > 256) {
        throw std::runtime_error("Huffman error: invalid symbol count.");
    }

    std::array<bool, 256> seen{};
    std::uint64_t frequencyTotal = 0;
    for (std::uint16_t index = 0; index < symbolCount; ++index) {
        char rawSymbol = 0;
        if (!input.get(rawSymbol)) {
            throw std::runtime_error("Huffman error: truncated frequency table.");
        }
        const auto symbol = static_cast<std::uint8_t>(static_cast<unsigned char>(rawSymbol));
        if (seen[symbol]) {
            throw std::runtime_error("Huffman error: duplicate symbol in frequency table.");
        }

        const std::uint64_t frequency = readUint64(input);
        if (frequency == 0 || frequency > originalSize - frequencyTotal) {
            throw std::runtime_error("Huffman error: invalid symbol frequency.");
        }
        seen[symbol] = true;
        frequencies[symbol] = frequency;
        frequencyTotal += frequency;
    }

    if (frequencyTotal != originalSize || ((originalSize == 0) != (symbolCount == 0))) {
        throw std::runtime_error("Huffman error: inconsistent frequency table.");
    }
    return originalSize;
}

void requireEndOfFile(std::istream& input) {
    char extraByte = 0;
    if (input.get(extraByte)) {
        throw std::runtime_error("Huffman error: unexpected trailing payload.");
    }
    if (!input.eof()) {
        throw std::runtime_error("Huffman error: failed while reading compressed file.");
    }
}

}  // namespace

void compress_huffman(const std::string& inputPath, const std::string& outputPath) {
    std::ifstream input(inputPath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Huffman error: cannot open input file: " + inputPath);
    }

    Frequencies frequencies{};
    std::uint64_t originalSize = 0;
    char rawByte = 0;
    while (input.get(rawByte)) {
        if (originalSize == std::numeric_limits<std::uint64_t>::max()) {
            throw std::runtime_error("Huffman error: input file is too large.");
        }
        ++frequencies[static_cast<unsigned char>(rawByte)];
        ++originalSize;
    }
    if (!input.eof()) {
        throw std::runtime_error("Huffman error: failed while reading input file.");
    }

    std::vector<std::unique_ptr<Node>> nodes;
    Node* root = buildTree(frequencies, nodes);
    CodeTable codes;
    if (root != nullptr) {
        std::vector<bool> path;
        buildCodes(root, path, codes);
    }

    std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Huffman error: cannot open output file: " + outputPath);
    }
    writeHeader(output, originalSize, frequencies);

    if (root != nullptr && !root->isLeaf()) {
        input.clear();
        input.seekg(0, std::ios::beg);
        if (!input) {
            throw std::runtime_error("Huffman error: cannot rewind input file.");
        }

        BitWriter writer(output);
        while (input.get(rawByte)) {
            const auto& code = codes[static_cast<unsigned char>(rawByte)];
            for (bool bit : code) {
                writer.writeBit(bit);
            }
        }
        if (!input.eof()) {
            throw std::runtime_error("Huffman error: failed while reading input file.");
        }
        writer.flush();
    }

    output.flush();
    if (!output) {
        throw std::runtime_error("Huffman error: failed to finish output file.");
    }
}

void decompress_huffman(const std::string& inputPath, const std::string& outputPath) {
    std::ifstream input(inputPath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Huffman error: cannot open input file: " + inputPath);
    }

    Frequencies frequencies{};
    const std::uint64_t originalSize = readHeader(input, frequencies);
    std::vector<std::unique_ptr<Node>> nodes;
    Node* root = buildTree(frequencies, nodes);

    std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Huffman error: cannot open output file: " + outputPath);
    }

    if (originalSize == 0) {
        requireEndOfFile(input);
        return;
    }

    if (root->isLeaf()) {
        requireEndOfFile(input);
        std::array<char, 8192> block{};
        block.fill(static_cast<char>(root->symbol));
        std::uint64_t remaining = originalSize;
        while (remaining != 0) {
            const auto amount = static_cast<std::streamsize>(
                std::min<std::uint64_t>(remaining, block.size())
            );
            output.write(block.data(), amount);
            remaining -= static_cast<std::uint64_t>(amount);
        }
    } else {
        BitReader reader(input);
        Frequencies decodedFrequencies{};
        for (std::uint64_t produced = 0; produced < originalSize; ++produced) {
            Node* current = root;
            while (!current->isLeaf()) {
                bool bit = false;
                if (!reader.readBit(bit)) {
                    throw std::runtime_error("Huffman error: truncated compressed payload.");
                }
                current = bit ? current->right : current->left;
                if (current == nullptr) {
                    throw std::runtime_error("Huffman error: invalid compressed payload.");
                }
            }
            output.put(static_cast<char>(current->symbol));
            ++decodedFrequencies[static_cast<std::size_t>(current->symbol)];
        }

        if (decodedFrequencies != frequencies) {
            throw std::runtime_error("Huffman error: payload does not match frequency table.");
        }
        if (!reader.isAtEndWithZeroPadding()) {
            throw std::runtime_error("Huffman error: invalid padding or trailing payload.");
        }
    }

    output.flush();
    if (!output) {
        throw std::runtime_error("Huffman error: failed to finish decompressed file.");
    }
}
