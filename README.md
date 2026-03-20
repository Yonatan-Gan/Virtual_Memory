Virtual Memory Simulator

A C++ simulator of an Operating System's virtual memory manager. This project was built to model how an OS handles memory at the hardware-software boundary, including translating virtual addresses to physical RAM and managing swap space when memory is full.

## 🧠 Core OS Concepts Implemented
* **Hierarchical Page Tables:** Utilizes a multi-level tree data structure to manage virtual-to-physical address translation efficiently.
* **On-Demand Paging:** Frames are allocated dynamically only when accessed, minimizing initial memory overhead.
* **Page Replacement Algorithm:** Implements a custom eviction algorithm using cyclic distance calculations to identify and swap out the optimal page when physical RAM is full.
* **Low-Level Bitwise Operations:** Parses virtual addresses into page numbers and offsets rapidly using bit shifts and masks.

## 🛠️ Technologies & Specifications
* **Language:** C++11
* **Build System:** Make
* **Architecture:** Simulates configurable word sizes, offset widths, and memory depths via `MemoryConstants.h`.

## 💻 How to Build and Run
This project includes a `Makefile` configured to build the core implementation into a static library or compile test executables[cite: 2].

1. Clone the repository.
2. Open your terminal in the project directory.
3. To build the static library (`libVirtualMemory.a`):
   ```bash
   make
