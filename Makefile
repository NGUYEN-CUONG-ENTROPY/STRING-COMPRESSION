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

# Ban build tam thoi chi gom RLE, dung khi huffman/lzw/arithmetic chua hoan thanh.
RLE_SRCS  = $(SRCDIR)/main.cpp $(SRCDIR)/rle.cpp $(SRCDIR)/utils.cpp

all: $(EXEC)

$(EXEC): $(SRCS) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

rle: $(RLE_SRCS)
	$(CXX) $(CXXFLAGS) -DRLE_ONLY $^ -o $(EXEC)

clean:
	rm -f $(BUILDDIR)/*.exe $(BUILDDIR)/*.o

# Kiem thu RLE: nen roi giai nen tung file trong tests/input va so sanh voi ban goc.
test-rle:
	@echo "Running RLE round-trip tests..."
	@for f in $(SRCDIR)/tests/input/*.txt; do \
		n=`basename $$f .txt`; \
		$(EXEC) -a rle -m c -i $$f -o $(BUILDDIR)/$$n.rle > /dev/null && \
		$(EXEC) -a rle -m d -i $(BUILDDIR)/$$n.rle -o $(BUILDDIR)/$$n.out > /dev/null && \
		if cmp -s $$f $(BUILDDIR)/$$n.out; then echo "PASS $$n"; else echo "FAIL $$n"; fi; \
	done

test: $(EXEC)
	@echo "Running tests..."
	$(EXEC) -a rle -m c -i $(SRCDIR)/tests/input/puzzle.txt -o $(BUILDDIR)/test_out.rle
	$(EXEC) -a rle -m d -i $(BUILDDIR)/test_out.rle -o $(BUILDDIR)/test_decomp.txt
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

.PHONY: all rle clean test test-rle
