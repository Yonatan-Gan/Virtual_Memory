CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -O2
AR = ar
ARFLAGS = rcs

LIBNAME = libVirtualMemory.a

# Default target
all: $(LIBNAME)

# Archive the core implementation into a static lib
$(LIBNAME): VirtualMemory.o
	$(AR) $(ARFLAGS) $@ $^

# Compile generic object (for debugging only)
VirtualMemory.o: VirtualMemory.cpp VirtualMemory.h MemoryConstants.h PhysicalMemory.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ---------------------------------------------------
# TEST 1
# ---------------------------------------------------
test1: test1_write_read_all_virtual_memory.cpp VirtualMemory_test1.o PhysicalMemory_test1.o
	$(CXX) $(CXXFLAGS) -o $@ $^

VirtualMemory_test1.o: VirtualMemory.cpp VirtualMemory.h PhysicalMemory.h MemoryConstants_test1.h
	$(CXX) $(CXXFLAGS) -DUSE_TEST_CONSTANTS -include MemoryConstants_test1.h -c VirtualMemory.cpp -o $@

PhysicalMemory_test1.o: PhysicalMemory.cpp PhysicalMemory.h MemoryConstants_test1.h
	$(CXX) $(CXXFLAGS) -DUSE_TEST_CONSTANTS -include MemoryConstants_test1.h -c PhysicalMemory.cpp -o $@

# ---------------------------------------------------
# TEST 2
# ---------------------------------------------------

test2: test2_write_one_page_twice_and_read.cpp VirtualMemory_test2.o PhysicalMemory_test2.o
	$(CXX) $(CXXFLAGS) -o $@ $^

VirtualMemory_test2.o: VirtualMemory.cpp VirtualMemory.h PhysicalMemory.h MemoryConstants_test2.h
	$(CXX) $(CXXFLAGS) -DUSE_TEST_CONSTANTS -include MemoryConstants_test2.h -c VirtualMemory.cpp -o $@

PhysicalMemory_test2.o: PhysicalMemory.cpp PhysicalMemory.h MemoryConstants_test2.h
	$(CXX) $(CXXFLAGS) -DUSE_TEST_CONSTANTS -include MemoryConstants_test2.h -c PhysicalMemory.cpp -o $@

# ---------------------------------------------------
# Debug builds using default constants
# ---------------------------------------------------

debug: debug_test
	./debug_test

debug_test: debug_test.cpp VirtualMemory.o PhysicalMemory.o
	$(CXX) $(CXXFLAGS) -o $@ $^

PhysicalMemory.o: PhysicalMemory.cpp PhysicalMemory.h MemoryConstants.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

simple: simple_debug
	./simple_debug

simple_debug: simple_debug.cpp VirtualMemory.o PhysicalMemory.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# ---------------------------------------------------
# Clean up everything
# ---------------------------------------------------

clean:
	rm -f *.o *.a test1 test2
