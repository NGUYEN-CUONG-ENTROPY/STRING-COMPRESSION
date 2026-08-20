STRING COMPRESSION - Group Project
CSC10004 - Summer 2026
====================================

1. Build:

   Cach 1 - dung Makefile:
   > make
   Output: build/compressor.exe

   Cach 2 - goi truc tiep g++ (dung lenh cua de bai):
   > g++ source/main.cpp source/rle.cpp source/huffman.cpp source/lzw.cpp \
         source/arithmetic.cpp source/bit_io.cpp source/utils.cpp \
         -o compressor.exe -std=c++17

   Ban build chi co RLE (khi cac module khac chua xong):
   > make rle
   hoac:
   > g++ source/main.cpp source/rle.cpp source/utils.cpp -o build/compressor.exe \
         -std=c++17 -O2 -DRLE_ONLY

   Luu y (Windows/MinGW): neu chay file .exe bao thieu DLL, hay them
   thu muc bin cua MinGW vao PATH, hoac build kem tuy chon -static.

2. Usage:
   compressor.exe -a <algorithm> -m <mode> -i <input_file> -o <output_file>

   Algorithms: rle, huff, lzw, arith
   Modes:     c (compress), d (decompress)
   -h, --help hien thi huong dan su dung

3. Examples:
   compressor.exe -a huff -m c -i input.txt -o output.huff
   compressor.exe -a huff -m d -i output.huff -o decompressed.txt

   Ket qua in ra man hinh (vi du voi puzzle.txt = "AAAAABBBBBCCCCCDDDDD"):
   Compression complete.
   --------------------------------
   Algorithm: RLE
   Execution Time: 0.05 ms
   Original Size: 20 bytes
   Compressed Size: 8 bytes
   Compression Ratio: 2.50
   Space Savings: 60.0%

4. Tests:
   Edge case tests: tests/input/
     empty.txt        file rong
     single_char.txt  dung 1 ky tu
     puzzle.txt       vi du trong de bai (20 byte -> 8 byte)
     no_repeat.txt    khong co ky tu lap (RLE nen am, 26 -> 52 byte)
     long_run.txt     chuoi lap dai hon 255 ky tu (kiem tra tran so dem)
     mixed.txt        co khoang trang, tab, xuong dong, chu so
   Expected outputs: tests/expected/ (file .rle tuong ung do RLE sinh ra)

   Chay bo test round-trip cua RLE:
   > make test-rle
   The Huffman implementation supports empty, single-symbol, ASCII, and binary files.
   Run the included round-trip test via:
   > make test

5. Dinh dang file nen cua RLE:
   Day cac cap 2 byte [count][value], khong co header.
     count: so lan lap, 1..255
     value: byte duoc lap
   Chuoi lap dai hon 255 duoc cat thanh nhieu cap lien tiep.
   Vi du: 300 ky tu 'A' -> FF 41 2D 41 (255 + 45 lan).

6. Experiments:
   - Test data:    ../experimental/data/
   - Artifacts:    ../experimental/artifacts/{algorithm}/
   - Results:      ../experimental/results/
   - Scripts:      ../experimental/gen_test_data.py (generate test files)
                   ../experimental/run_experiments.py (auto benchmark)

   Chay day du (4 algorithms, 2 scenarios, verify SHA-256 moi trial):
   > py experimental/run_experiments.py --build --regenerate-data --repeats 5 --warmups 1

   Bang co the dua vao report:
   > experimental/results/report_table.md

   Huong dan va tuy chon chi tiet:
   > experimental/README.md
