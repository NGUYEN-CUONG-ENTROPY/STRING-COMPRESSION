# STRING COMPRESSION

**CSC10004 — Data Structures & Algorithms — Summer 2026**

Đồ án nhóm: Xây dựng công cụ nén văn bản không mất dữ liệu (lossless) bằng C++.

---

## 📁 Cấu trúc thư mục

```
STRING-COMPRESSION/
│
├── source/                          # Mã nguồn C++
│   ├── main.cpp                     # Entry point, dispatch CLI
│   ├── rle.h / rle.cpp              # Run-Length Encoding
│   ├── huffman.h / cpp              # Huffman Coding
│   ├── lzw.h / lzw.cpp              # Lempel-Ziv-Welch
│   ├── arithmetic.h / cpp           # Arithmetic Coding (Bonus)
│   ├── bit_io.h / bit_io.cpp        # Đọc/ghi bit
│   ├── utils.h / utils.cpp          # Timer + metrics
│   ├── readme.txt                   # Hướng dẫn build & sử dụng chi tiết
│   └── tests/
│       ├── input/                   # Test edge cases nhỏ (empty, single_char...)
│       └── expected/                # File output kỳ vọng để verify
│
├── build/                           # Binary và object files (tạo khi build)
│
├── experimental/                    # Dữ liệu & kết quả thực nghiệm
│   ├── data/
│   │   ├── scenario1_size/          # English text: 10KB → 10MB
│   │   └── scenario2_entropy/       # repetitive, English, random (1MB)
│   ├── compressed/                  # File đã nén
│   │   ├── rle/ / huff/ / lzw/ / arith/
│   ├── decompressed/                # File giải nén (verify lossless)
│   │   ├── rle/ / huff/ / lzw/ / arith/
│   ├── gen_test_data.py             # Script sinh file test
│   └── run_experiments.py           # Script chạy benchmark tự động
│
├── Makefile                         # Build script
├── Report.pdf                       # Báo cáo đồ án
└── video.txt                        # Link YouTube demo
```

---

## 🔧 Build

```bash
make
```

Kết quả: `build/compressor.exe`

Dọn dẹp:

```bash
make clean
```

---

## 🚀 Usage

```bash
compressor.exe -a <algorithm> -m <mode> -i <input> -o <output>
```

| Option | Giá trị | Mô tả |
|--------|---------|-------|
| `-a` | `rle` / `huff` / `lzw` / `arith` | Thuật toán |
| `-m` | `c` (compress) / `d` (decompress) | Chế độ |
| `-i` | path | File đầu vào |
| `-o` | path | File đầu ra |

### Ví dụ

```bash
# Nén bằng Huffman
compressor.exe -a huff -m c -i input.txt -o output.huff

# Giải nén bằng Huffman
compressor.exe -a huff -m d -i output.huff -o decompressed.txt
```

Huffman làm việc theo byte và hỗ trợ cả file rỗng, file chỉ có một ký tự,
ký tự ASCII mở rộng và dữ liệu nhị phân. File `.huff` chứa chữ ký `HUF1`, kích
thước ban đầu, bảng tần suất và bitstream đã mã hóa, nên có thể tự giải nén mà
không cần lưu cây Huffman riêng.

Chạy kiểm thử round-trip có sẵn:

```bash
make test
```

---

## 🧪 Các thuật toán

| Thuật toán | File nguồn | Ghi chú |
|------------|-----------|---------|
| RLE        | `rle.h / rle.cpp` | Run-Length Encoding |
| Huffman    | `huffman.h / huffman.cpp` | Huffman Coding |
| LZW        | `lzw.h / lzw.cpp` | Lempel-Ziv-Welch |
| Arithmetic | `arithmetic.h / arithmetic.cpp` | Arithmetic Coding (Bonus) |

---

## Thực nghiệm

Luồng dữ liệu khi chạy experiment:
```
experimental/data/*.txt
  -> compressor.exe -m c
    -> experimental/compressed/{algorithm}/*.bin
      -> compressor.exe -m d
        -> experimental/decompressed/{algorithm}/*.txt
          -> diff voi file goc (verify lossless)
```

**Scenario 1** - Anh huong cua kich thuoc file: co dinh English text, chay tu 10 KB den 10 MB.
- Input: `experimental/data/scenario1_size/`
- Chay tung thuat toan, ghi vao `experimental/compressed/` va `decompressed/`

**Scenario 2** - Anh huong cua entropy: co dinh 1 MB, chay tren 3 loai du lieu.
- Input: `experimental/data/scenario2_entropy/`

Sinh du lieu va benchmark tu dong:
```bash
python experimental/gen_test_data.py
python experimental/run_experiments.py
```

---

## Nop bai

Nén thành `<GroupID>.zip` gồm:
```
source/         # Code + tests
Report.pdf      # Báo cáo
video.txt       # Link YouTube
```
