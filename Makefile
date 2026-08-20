CXX       = g++
CXXFLAGS  = -std=c++17 -O2 -Wall
SRCDIR    = source
BUILDDIR  = build
EXEC      = $(BUILDDIR)/compressor.exe

SRCS      = $(SRCDIR)/main.cpp       \
            $(SRCDIR)/rle.cpp        \
            $(SRCDIR)/huffman.cpp    \
            $(SRCDIR)/lzw.cpp        \
            $(SRCDIR)/arithmetic.cpp \
            $(SRCDIR)/bit_io.cpp     \
            $(SRCDIR)/utils.cpp

all: $(EXEC)

$(EXEC): $(SRCS) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -f $(BUILDDIR)/*.exe $(BUILDDIR)/*.o

test: $(EXEC)
	@echo "Running Huffman round-trip tests..."
	$(EXEC) -a huff -m c -i $(SRCDIR)/tests/english_10kb.txt -o $(BUILDDIR)/test_out.huff
	$(EXEC) -a huff -m d -i $(BUILDDIR)/test_out.huff -o $(BUILDDIR)/test_decomp.txt
	cmp $(SRCDIR)/tests/english_10kb.txt $(BUILDDIR)/test_decomp.txt
	$(EXEC) -a huff -m c -i $(SRCDIR)/tests/repetitive_sample.txt -o $(BUILDDIR)/test_repetitive.huff
	$(EXEC) -a huff -m d -i $(BUILDDIR)/test_repetitive.huff -o $(BUILDDIR)/test_repetitive.txt
	cmp $(SRCDIR)/tests/repetitive_sample.txt $(BUILDDIR)/test_repetitive.txt
	$(EXEC) -a huff -m c -i $(SRCDIR)/tests/empty.txt -o $(BUILDDIR)/test_empty.huff
	$(EXEC) -a huff -m d -i $(BUILDDIR)/test_empty.huff -o $(BUILDDIR)/test_empty.txt
	cmp $(SRCDIR)/tests/empty.txt $(BUILDDIR)/test_empty.txt
	@echo "All Huffman round-trip tests passed."

.PHONY: all clean test
