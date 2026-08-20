#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <string>

namespace {

bool filesEqual(const std::string& firstPath, const std::string& secondPath) {
    std::ifstream first(firstPath, std::ios::binary);
    std::ifstream second(secondPath, std::ios::binary);
    if (!first || !second) {
        return false;
    }

    std::array<char, 8192> firstBuffer{};
    std::array<char, 8192> secondBuffer{};
    do {
        first.read(firstBuffer.data(), static_cast<std::streamsize>(firstBuffer.size()));
        second.read(secondBuffer.data(), static_cast<std::streamsize>(secondBuffer.size()));
        if (first.gcount() != second.gcount()) {
            return false;
        }
        if (!std::equal(firstBuffer.begin(), firstBuffer.begin() + first.gcount(),
                        secondBuffer.begin())) {
            return false;
        }
    } while (first.gcount() != 0);

    return first.eof() && second.eof();
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: compare_files <first> <second>\n";
        return 2;
    }

    if (!filesEqual(argv[1], argv[2])) {
        std::cerr << "Files differ: " << argv[1] << " and " << argv[2] << '\n';
        return 1;
    }
    return 0;
}
