# Experimental benchmark

Bộ script này chạy đúng hai scenario trong `Project_Compression.pdf` cho bốn
thuật toán RLE, Huffman, LZW và Arithmetic Coding.

## Chạy benchmark đầy đủ

Yêu cầu: Python 3.10+ và `g++` hỗ trợ C++17 trong `PATH`.

Trên Windows:

```powershell
py experimental/run_experiments.py --build --regenerate-data --repeats 5 --warmups 1
```

Trên Linux/macOS, thay `py` bằng `python3`.

Lệnh trên tự động:

1. Build `build/compressor.exe` bằng `g++ -std=c++17 -O2 -Wall`.
2. Sinh dữ liệu ASCII có seed cố định và kích thước byte chính xác.
3. Chạy một warm-up rồi đo năm lần cho mỗi cặp dataset/thuật toán.
4. Giải nén và so SHA-256 với file gốc ở mọi lần đo.
5. Xuất số liệu thô, số liệu tổng hợp theo median và bảng cho report.

Có thể chạy riêng một scenario hoặc một số thuật toán:

```powershell
py experimental/run_experiments.py --scenario size --algorithms rle huff
py experimental/run_experiments.py --scenario entropy --repeats 10 --warmups 2
```

Nếu đã build và sinh dữ liệu trước đó, không cần `--build` hoặc
`--regenerate-data`. Xem toàn bộ tùy chọn bằng:

```powershell
py experimental/run_experiments.py --help
```

## Datasets

`gen_test_data.py` chỉ dùng Python standard library và sinh dữ liệu xác định từ
seed mặc định `20260820`:

- Scenario 1: English-like text ở 10 KiB, 100 KiB, 1 MiB và 10 MiB.
- Scenario 2: highly repetitive, English-like và random printable ASCII, đều
  có kích thước 1 MiB.

Nhãn KB/MB trong bảng bám theo đề bài; kích thước thực dùng 1 KiB = 1024 byte và
1 MiB = 1,048,576 byte. `experimental/data/manifest.csv` lưu kích thước, entropy
Shannon và SHA-256 của từng input để tái lập thí nghiệm.

## Kết quả

Các file được sinh trong `experimental/results/`:

- `raw_results.csv`: từng trial, gồm thời gian CLI và wall time.
- `summary.csv`: median, mean, standard deviation, min/max, ratio, savings,
  throughput và thời gian giải nén.
- `report_table.md`: bảng `time (ratio)` có thể chèn vào Section 3.4.
- `run_metadata.json`: command, seed, hệ điều hành, Python và checksum executable.

File nén/giải nén dùng để kiểm chứng nằm trong `experimental/artifacts/`. Các thư
mục dữ liệu sinh tự động, artifacts và results đều được `.gitignore` bỏ qua.

`compression_execution_ms` là thời gian do chính CLI C++ in ra. Với code hiện
tại, timer bao quanh toàn bộ lời gọi thuật toán (đọc, nén và ghi file), nhưng
không gồm thời gian khởi động process. `compression_wall_ms` được Python đo riêng
và có gồm thời gian khởi động process. Khi viết bảng report, dùng
`compression_ms_median` để nhất quán với output theo yêu cầu đề bài.

## Vẽ biểu đồ (tùy chọn)

```powershell
py -m pip install matplotlib
py experimental/plot_results.py
```

Script tạo bốn PNG 200 DPI trong `experimental/results/charts/`: thời gian và
compression ratio cho từng scenario.

