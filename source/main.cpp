#include "huffman.h"
#include "lzw.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

namespace fs = std::filesystem;

struct Arguments {
    std::string algorithm;
    std::string mode;
    std::string inputPath;
    std::string outputPath;
};

void printUsage(std::ostream& output) {
    output << "Usage: compressor[.exe] -a <algorithm> -m <mode> -i <input_file> -o <output_file>\n"
           << "  -a  Algorithm: huff or lzw\n"
           << "  -m  Mode: c (compress) or d (decompress)\n"
           << "  -i  Input file path\n"
           << "  -o  Output file path\n";
}

void assignOption(std::string& destination,
                  const std::string& value,
                  const std::string& option) {
    if (!destination.empty()) {
        throw std::invalid_argument("Duplicate option: " + option);
    }
    if (value.empty()) {
        throw std::invalid_argument("Empty value for option: " + option);
    }
    destination = value;
}

Arguments parseArguments(int argc, char* argv[]) {
    if (argc != 9) {
        throw std::invalid_argument("Expected exactly four option-value pairs.");
    }

    Arguments arguments;
    for (int index = 1; index < argc; index += 2) {
        const std::string option = argv[index];
        const std::string value = argv[index + 1];
        if (option == "-a") {
            assignOption(arguments.algorithm, value, option);
        } else if (option == "-m") {
            assignOption(arguments.mode, value, option);
        } else if (option == "-i") {
            assignOption(arguments.inputPath, value, option);
        } else if (option == "-o") {
            assignOption(arguments.outputPath, value, option);
        } else {
            throw std::invalid_argument("Unknown option: " + option);
        }
    }

    if (arguments.algorithm.empty() || arguments.mode.empty()
        || arguments.inputPath.empty() || arguments.outputPath.empty()) {
        throw std::invalid_argument("Options -a, -m, -i and -o are all required.");
    }
    if (arguments.algorithm != "huff" && arguments.algorithm != "lzw") {
        throw std::invalid_argument("Unsupported algorithm: " + arguments.algorithm);
    }
    if (arguments.mode != "c" && arguments.mode != "d") {
        throw std::invalid_argument("Mode must be c or d.");
    }

    const fs::path input = fs::absolute(arguments.inputPath).lexically_normal();
    const fs::path output = fs::absolute(arguments.outputPath).lexically_normal();
    if (input == output) {
        throw std::invalid_argument("Input and output paths must be different.");
    }
    return arguments;
}

std::string displayAlgorithm(const std::string& algorithm) {
    return algorithm == "huff" ? "Huffman" : "LZW";
}

void runAlgorithm(const Arguments& arguments) {
    if (arguments.algorithm == "huff") {
        if (arguments.mode == "c") {
            compress_huffman(arguments.inputPath, arguments.outputPath);
        } else {
            decompress_huffman(arguments.inputPath, arguments.outputPath);
        }
    } else if (arguments.mode == "c") {
        compress_lzw(arguments.inputPath, arguments.outputPath);
    } else {
        decompress_lzw(arguments.inputPath, arguments.outputPath);
    }
}

void printSummary(const Arguments& arguments,
                  double elapsedMilliseconds,
                  std::uintmax_t inputSize,
                  std::uintmax_t outputSize) {
    std::cout << (arguments.mode == "c" ? "Compression" : "Decompression")
              << " complete.\n"
              << "--------------------------------\n"
              << "Algorithm: " << displayAlgorithm(arguments.algorithm) << '\n'
              << std::fixed << std::setprecision(3)
              << "Execution Time: " << elapsedMilliseconds << " ms\n";

    if (arguments.mode == "c") {
        std::cout << "Original Size: " << inputSize << " bytes\n"
                  << "Compressed Size: " << outputSize << " bytes\n";
        if (inputSize == 0 || outputSize == 0) {
            std::cout << "Compression Ratio: N/A\n"
                      << "Space Savings: N/A\n";
        } else {
            const double ratio = static_cast<double>(inputSize)
                                 / static_cast<double>(outputSize);
            const double savings = (1.0 - static_cast<double>(outputSize)
                                           / static_cast<double>(inputSize)) * 100.0;
            std::cout << "Compression Ratio: " << ratio << '\n'
                      << "Space Savings: " << savings << "%\n";
        }
    } else {
        std::cout << "Compressed Size: " << inputSize << " bytes\n"
                  << "Decompressed Size: " << outputSize << " bytes\n";
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        printUsage(std::cout);
        return 0;
    }

    try {
        const Arguments arguments = parseArguments(argc, argv);
        if (!fs::is_regular_file(arguments.inputPath)) {
            throw std::runtime_error("Input file does not exist or is not a regular file: "
                                     + arguments.inputPath);
        }

        const std::uintmax_t inputSize = fs::file_size(arguments.inputPath);
        const auto start = std::chrono::steady_clock::now();
        runAlgorithm(arguments);
        const auto end = std::chrono::steady_clock::now();
        const std::uintmax_t outputSize = fs::file_size(arguments.outputPath);
        const double elapsedMilliseconds =
            std::chrono::duration<double, std::milli>(end - start).count();

        printSummary(arguments, elapsedMilliseconds, inputSize, outputSize);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n\n";
        printUsage(std::cerr);
        return 1;
    }
}
