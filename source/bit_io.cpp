#include "bit_io.h"

#include <istream>
#include <ostream>
#include <stdexcept>

BitWriter::BitWriter(std::ostream& output) : output_(output) {}

void BitWriter::writeBit(bool bit) {
    buffer_ = static_cast<std::uint8_t>((buffer_ << 1U) | (bit ? 1U : 0U));
    ++bitCount_;

    if (bitCount_ == 8) {
        output_.put(static_cast<char>(buffer_));
        if (!output_) {
            throw std::runtime_error("Huffman error: failed to write compressed data.");
        }
        buffer_ = 0;
        bitCount_ = 0;
    }
}

void BitWriter::flush() {
    if (bitCount_ != 0) {
        buffer_ = static_cast<std::uint8_t>(buffer_ << (8 - bitCount_));
        output_.put(static_cast<char>(buffer_));
        if (!output_) {
            throw std::runtime_error("Huffman error: failed to flush compressed data.");
        }
        buffer_ = 0;
        bitCount_ = 0;
    }
}

BitReader::BitReader(std::istream& input) : input_(input) {}

bool BitReader::readBit(bool& bit) {
    if (bitsRemaining_ == 0) {
        char byte = 0;
        if (!input_.get(byte)) {
            return false;
        }
        buffer_ = static_cast<std::uint8_t>(static_cast<unsigned char>(byte));
        bitsRemaining_ = 8;
    }

    bit = (buffer_ & 0x80U) != 0;
    buffer_ = static_cast<std::uint8_t>(buffer_ << 1U);
    --bitsRemaining_;
    return true;
}

bool BitReader::isAtEndWithZeroPadding() {
    if (buffer_ != 0) {
        return false;
    }

    char extraByte = 0;
    if (input_.get(extraByte)) {
        return false;
    }
    return input_.eof();
}
