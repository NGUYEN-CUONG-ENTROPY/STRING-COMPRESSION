CXX       = g++
CXXFLAGS  = -std=c++17 -O2 -Wall
SRCDIR    = source
BUILDDIR  = build
EXEC      = $(BUILDDIR)/compressor.exe
RLE_EXEC  = $(BUILDDIR)/compressor_rle.exe
COMPARE   = $(BUILDDIR)/compare_files.exe
NULLDEV   = /dev/null

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

$(COMPARE): $(SRCDIR)/tests/compare_files.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(RLE_EXEC): $(RLE_SRCS) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -DRLE_ONLY $^ -o $@

rle: $(RLE_EXEC)

clean:
	rm -f $(BUILDDIR)/*.exe $(BUILDDIR)/*.o

# Kiem thu RLE: nen roi giai nen tung file trong tests/input va so sanh voi ban goc.
test-rle: $(EXEC) $(COMPARE)
	@echo "Running RLE round-trip tests..."
	@failed=0; for f in $(SRCDIR)/tests/input/*.txt; do \
		n=$${f##*/}; n=$${n%.txt}; \
		if $(EXEC) -a rle -m c -i "$$f" -o $(BUILDDIR)/$$n.rle > $(NULLDEV) && \
		   $(EXEC) -a rle -m d -i $(BUILDDIR)/$$n.rle -o $(BUILDDIR)/$$n.rle.out > $(NULLDEV) && \
		   $(COMPARE) "$$f" $(BUILDDIR)/$$n.rle.out > $(NULLDEV); \
		then echo "PASS $$n"; else echo "FAIL $$n"; failed=1; fi; \
	done; exit $$failed

test-huffman: $(EXEC) $(COMPARE)
	@echo "Running Huffman round-trip tests..."
	@failed=0; for f in $(SRCDIR)/tests/input/*.txt; do \
		n=$${f##*/}; n=$${n%.txt}; \
		if $(EXEC) -a huff -m c -i "$$f" -o $(BUILDDIR)/$$n.huff > $(NULLDEV) && \
		   $(EXEC) -a huff -m d -i $(BUILDDIR)/$$n.huff -o $(BUILDDIR)/$$n.huff.out > $(NULLDEV) && \
		   $(COMPARE) "$$f" $(BUILDDIR)/$$n.huff.out > $(NULLDEV); \
		then echo "PASS $$n"; else echo "FAIL $$n"; failed=1; fi; \
	done; exit $$failed

test: test-rle test-huffman

.PHONY: all rle clean test test-rle test-huffman
