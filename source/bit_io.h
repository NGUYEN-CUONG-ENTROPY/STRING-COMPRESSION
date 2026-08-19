#ifndef BIT_IO_H
#define BIT_IO_H

#include <fstream>
#include <cstdint>

// MOCK (STUB) CLASS - Chờ Tấn code logic thật vào đây
class BitWriter {
public:
    BitWriter(std::ofstream& out_stream) {}
    void write_bit(uint8_t bit) {}
    void flush() {}
};

// MOCK (STUB) CLASS - Chờ Tấn code logic thật vào đây
class BitReader {
public:
    BitReader(std::ifstream& in_stream) {}
    int read_bit() { return 0; }
};

#endif // BIT_IO_H
