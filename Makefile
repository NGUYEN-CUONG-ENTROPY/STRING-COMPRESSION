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

$(EXEC): $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(BUILDDIR)/*.exe $(BUILDDIR)/*.o

test: $(EXEC)
	@echo "Running tests..."
	$(EXEC) -a rle -m c -i $(SRCDIR)/tests/input/english_10KB.txt -o $(BUILDDIR)/test_out.rle
	$(EXEC) -a rle -m d -i $(BUILDDIR)/test_out.rle -o $(BUILDDIR)/test_decomp.txt

.PHONY: all clean test
