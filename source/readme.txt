STRING COMPRESSION - Group Project
CSC10004 - Summer 2026
====================================

1. Build:
   > make
   Output: build/compressor.exe

2. Usage:
   compressor.exe -a <algorithm> -m <mode> -i <input_file> -o <output_file>

   Algorithms: rle, huff, lzw, arith
   Modes:     c (compress), d (decompress)

3. Examples:
   compressor.exe -a rle -m c -i input.txt -o output.rle
   compressor.exe -a rle -m d -i output.rle -o decompressed.txt

4. Tests:
   Edge case tests: tests/input/ (small files like empty.txt, single_char.txt)
   Expected outputs: tests/expected/
   Run quick test via:
   > make test

5. Experiments:
   - Test data:    ../experimental/data/
   - Compressed:   ../experimental/compressed/{algorithm}/
   - Decompressed: ../experimental/decompressed/{algorithm}/
   - Scripts:      ../experimental/gen_test_data.py (generate test files)
                   ../experimental/run_experiments.py (auto benchmark)
