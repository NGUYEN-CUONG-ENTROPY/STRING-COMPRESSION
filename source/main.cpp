// ---------------------------------------------------------------------------
// compressor - cong cu nen/giai nen van ban khong mat du lieu.
//
//   compressor[.exe] -a [algorithm] -m [mode] -i [input_file] -o [output_file]
//
// main.cpp chi lo phan giao dien dong lenh: doc tham so, kiem tra hop le,
// goi dung module thuat toan, do thoi gian va in bang tong ket hieu nang.
// Moi module thuat toan tu lo phan doc/ghi file cua no va nem std::runtime_error
// khi gap loi.
// ---------------------------------------------------------------------------

#include <iostream>
#include <stdexcept>
#include <string>

#include "rle.h"
#include "utils.h"

// RLE_ONLY: build tam thoi chi gom RLE (dung khi cac module khac chua xong).
// Ban build day du (make) khong dinh nghia macro nay.
#ifndef RLE_ONLY
#include "arithmetic.h"
#include "huffman.h"
#include "lzw.h"
#endif

struct Options {
    std::string algorithm;
    std::string mode;
    std::string input_path;
    std::string output_path;
};

static void print_usage() {
    std::cout
        << "Usage: compressor[.exe] -a [algorithm] -m [mode] -i [input_file] -o [output_file]\n"
        << "\n"
        << "Options:\n"
        << "  -a [algorithm]   Select algorithm: rle, huff, lzw, arith\n"
        << "  -m [mode]        Select mode: c (compress), d (decompress)\n"
        << "  -i [input_file]  Path to the source file\n"
        << "  -o [output_file] Path to the resulting file\n"
        << "  -h, --help       Show this help message\n"
        << "\n"
        << "Example:\n"
        << "  compressor.exe -a rle -m c -i input.txt -o output.rle\n"
        << "  compressor.exe -a rle -m d -i output.rle -o restored.txt\n";
}

// Doc cac cap tham so dang "-x value". Nem std::runtime_error neu sai cu phap.
static Options parse_arguments(int argc, char* argv[]) {
    Options opts;

    for (int i = 1; i < argc; ++i) {
        std::string flag = argv[i];

        if (flag != "-a" && flag != "-m" && flag != "-i" && flag != "-o") {
            throw std::runtime_error("Tham so khong hop le: " + flag);
        }
        if (i + 1 >= argc) {
            throw std::runtime_error("Thieu gia tri cho tham so " + flag);
        }

        std::string value = argv[++i];
        if (flag == "-a") opts.algorithm = value;
        else if (flag == "-m") opts.mode = value;
        else if (flag == "-i") opts.input_path = value;
        else opts.output_path = value;
    }

    if (opts.algorithm.empty()) throw std::runtime_error("Thieu tham so -a [algorithm]");
    if (opts.mode.empty()) throw std::runtime_error("Thieu tham so -m [mode]");
    if (opts.input_path.empty()) throw std::runtime_error("Thieu tham so -i [input_file]");
    if (opts.output_path.empty()) throw std::runtime_error("Thieu tham so -o [output_file]");

    if (opts.algorithm != "rle" && opts.algorithm != "huff" &&
        opts.algorithm != "lzw" && opts.algorithm != "arith") {
        throw std::runtime_error("Thuat toan khong ho tro: " + opts.algorithm +
                                 " (chi nhan rle, huff, lzw, arith)");
    }
    if (opts.mode != "c" && opts.mode != "d") {
        throw std::runtime_error("Che do khong ho tro: " + opts.mode +
                                 " (chi nhan c hoac d)");
    }
    return opts;
}

// Ten thuat toan hien thi trong bang tong ket.
static std::string display_name(const std::string& algorithm) {
    if (algorithm == "rle") return "RLE";
    if (algorithm == "huff") return "Huffman";
    if (algorithm == "lzw") return "LZW";
    return "Arithmetic";
}

// Goi dung ham cua module thuat toan tuong ung.
static void run_algorithm(const Options& opts) {
    const bool compressing = (opts.mode == "c");

    if (opts.algorithm == "rle") {
        if (compressing) compress_rle(opts.input_path, opts.output_path);
        else decompress_rle(opts.input_path, opts.output_path);
    }
#ifndef RLE_ONLY
    else if (opts.algorithm == "huff") {
        if (compressing) compress_huffman(opts.input_path, opts.output_path);
        else decompress_huffman(opts.input_path, opts.output_path);
    }
    else if (opts.algorithm == "lzw") {
        if (compressing) compress_lzw(opts.input_path, opts.output_path);
        else decompress_lzw(opts.input_path, opts.output_path);
    }
    else {
        if (compressing) compress_arithmetic(opts.input_path, opts.output_path);
        else decompress_arithmetic(opts.input_path, opts.output_path);
    }
#else
    else {
        throw std::runtime_error("Thuat toan '" + opts.algorithm +
                                 "' khong co trong ban build RLE_ONLY.");
    }
#endif
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        print_usage();
        return 1;
    }
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        }
    }

    try {
        Options opts = parse_arguments(argc, argv);

        const long long input_size = get_file_size(opts.input_path);
        if (input_size < 0) {
            throw std::runtime_error("Khong doc duoc file dau vao: " + opts.input_path);
        }

        Timer timer;
        timer.start();
        run_algorithm(opts);
        const double elapsed = timer.elapsed_ms();

        const long long output_size = get_file_size(opts.output_path);
        if (output_size < 0) {
            throw std::runtime_error("Khong tao duoc file dau ra: " + opts.output_path);
        }

        if (opts.mode == "c") {
            print_compression_summary(display_name(opts.algorithm), elapsed, input_size, output_size);
        } else {
            print_decompression_summary(display_name(opts.algorithm), elapsed, input_size, output_size);
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n\n";
        print_usage();
        return 1;
    }
}
