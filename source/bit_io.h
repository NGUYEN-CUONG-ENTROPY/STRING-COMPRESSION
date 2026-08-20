#ifndef BIT_IO_H
#define BIT_IO_H

#include <cstdint>
#include <iosfwd>

class BitWriter
{
public:
    explicit BitWriter(std::ostream &output);

    void writeBit(bool bit);
    void flush();

private:
    std::ostream &output_;
    std::uint8_t buffer_ = 0;
    int bitCount_ = 0;
};

class BitReader
{
public:
    explicit BitReader(std::istream &input);

    // Returns false when no complete byte remains in the input stream.
    bool readBit(bool &bit);

private:
    std::istream &input_;
    std::uint8_t buffer_ = 0;
    int bitsRemaining_ = 0;
};

#endif
