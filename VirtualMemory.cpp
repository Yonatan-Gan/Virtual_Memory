#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <climits>
#include <cstdio>

// Memory management utilities
namespace MemoryUtils {

    // Extract page number from virtual address
    inline uint64_t extractPageFromVA(uint64_t virtualAddr) {
      return virtualAddr >> OFFSET_WIDTH;
    }

    // Extract offset from virtual address
    inline uint64_t extractOffsetFromVA(uint64_t virtualAddr) {
      return virtualAddr & ((1ULL << OFFSET_WIDTH) - 1);
    }

    // Get the relevant address part for a specific tree level (FIXED)
    uint64_t getAddressPart(uint64_t virtualAddr, int level) {
      int bitsToShift = VIRTUAL_ADDRESS_WIDTH - ((level + 1) * OFFSET_WIDTH);
      uint64_t mask = (1ULL << OFFSET_WIDTH) - 1;
      return (virtualAddr >> bitsToShift) & mask;
    }

    // Compute distance between pages (with wraparound)
    inline uint64_t computePageDistance(uint64_t pageA, uint64_t pageB) {
      uint64_t linearDist = (pageA > pageB) ? (pageA - pageB) : (pageB - pageA);
      uint64_t wrapDist = NUM_PAGES - linearDist;
      return (linearDist < wrapDist) ? linearDist : wrapDist;
    }

    // Physical memory access helpers
    inline word_t readPhysicalWord(uint64_t frameIdx, uint64_t wordOffset) {
      word_t result;
      PMread(frameIdx * PAGE_SIZE + wordOffset, &result);
      return result;
    }

    inline void writePhysicalWord(uint64_t frameIdx, uint64_t wordOffset, word_t val) {
      PMwrite(frameIdx * PAGE_SIZE + wordOffset, val);
    }

    // Initialize frame to all zeros
    void zeroOutFrame(uint64_t frameIdx) {
      for (uint64_t i = 0; i < PAGE_SIZE; i++) {
        writePhysicalWord(frameIdx, i, 0);
      }
    }
}

// Frame search and allocation system
class FrameManager {
 private:
  struct AllocationState {
      uint64_t highestFrameIndex;
      bool emptyTableFound;
      uint64_t availableTableFrame;
      uint64_t availableTableParent;
      uint64_t availableTableIndex;
      bool evictionReady;
      uint64_t targetEvictFrame;
      uint64_t targetEvictPage;
      uint64_t targetEvictParent;
      uint64_t targetEvictIndex;
      uint64_t bestEvictDistance;

      AllocationState() : highestFrameIndex(0), emptyTableFound(false),
                          evictionReady(false), bestEvictDistance(0) {}
  };

  // Recursively scan page table structure (FIXED)
  void traversePageStructure(uint64_t currentFrame, uint64_t parentFrame,
                             uint64_t indexInParent, uint64_t pageBase,
                             int currentDepth, uint64_t targetPage,
                             uint64_t excludeFrame, AllocationState& state) {

    if (currentFrame >= NUM_FRAMES) return;

    // Track highest frame index seen
    if (currentFrame > state.highestFrameIndex) {
      state.highestFrameIndex = currentFrame;
    }

    if (currentDepth == TABLES_DEPTH) {
      // This is a data page - evaluate for eviction
      uint64_t distance = MemoryUtils::computePageDistance(targetPage, pageBase);
      if (!state.evictionReady || distance > state.bestEvictDistance) {
        state.bestEvictDistance = distance;
        state.targetEvictFrame = currentFrame;
        state.targetEvictPage = pageBase;
        state.targetEvictParent = parentFrame;
        state.targetEvictIndex = indexInParent;
        state.evictionReady = true;
      }
      return;
    }

    // This is a page table - check if it's empty
    bool frameEmpty = true;
    for (uint64_t slot = 0; slot < PAGE_SIZE; slot++) {
      word_t entry = MemoryUtils::readPhysicalWord(currentFrame, slot);
      if (entry != 0) {
        frameEmpty = false;
        // FIXED: Consistent page number calculation
        uint64_t nextPageNumber = (pageBase << OFFSET_WIDTH) + slot;
        traversePageStructure(entry, currentFrame, slot, nextPageNumber,
                              currentDepth + 1, targetPage, excludeFrame, state);
      }
    }

    // Record empty table if found (but not the excluded frame)
    if (frameEmpty && currentDepth > 0 && currentFrame != excludeFrame
        && !state.emptyTableFound) {
      state.emptyTableFound = true;
      state.availableTableFrame = currentFrame;
      state.availableTableParent = parentFrame;
      state.availableTableIndex = indexInParent;
    }
  }

 public:
  // Find and allocate an available frame
  uint64_t acquireFrame(uint64_t targetPage, uint64_t currentTableFrame) {
    AllocationState allocation;

    // Scan entire page table tree starting from root
    traversePageStructure(0, 0, 0, 0, 0, targetPage, currentTableFrame, allocation);

    // Priority 1: Reuse empty page table
    if (allocation.emptyTableFound) {
      // Unlink the empty table from its parent
      MemoryUtils::writePhysicalWord(allocation.availableTableParent,
                                     allocation.availableTableIndex, 0);
      return allocation.availableTableFrame;
    }

    // Priority 2: Use next unused frame
    if (allocation.highestFrameIndex + 1 < NUM_FRAMES) {
      return allocation.highestFrameIndex + 1;
    }

    // Priority 3: Evict a page
    if (allocation.evictionReady) {
      PMevict(allocation.targetEvictFrame, allocation.targetEvictPage);
      MemoryUtils::writePhysicalWord(allocation.targetEvictParent,
                                     allocation.targetEvictIndex, 0);
      return allocation.targetEvictFrame;
    }

    return 0; // Should never happen
  }
};

// Page table navigation and management
class AddressResolver {
 private:
  FrameManager frameManager;

 public:
  // Navigate through page table hierarchy to find/create page (FIXED)
  uint64_t resolveVirtualAddress(uint64_t virtualAddress) {
    uint64_t currentTableFrame = 0; // Start at root
    bool needsRestore = false;
    uint64_t pageNumber = MemoryUtils::extractPageFromVA(virtualAddress);

    // Walk down the page table tree
    for (int level = 0; level < TABLES_DEPTH; level++) {
      uint64_t tableSlot = MemoryUtils::getAddressPart(virtualAddress, level);
      word_t nextFrame = MemoryUtils::readPhysicalWord(currentTableFrame, tableSlot);

      if (nextFrame == 0) {
        // Missing entry - need to allocate
        uint64_t newFrame = frameManager.acquireFrame(pageNumber, currentTableFrame);

        // Clear frame if it will contain a table (not a data page)
        if (level < TABLES_DEPTH - 1) {
          MemoryUtils::zeroOutFrame(newFrame);
        }

        MemoryUtils::writePhysicalWord(currentTableFrame, tableSlot, newFrame);
        currentTableFrame = newFrame;
        needsRestore = true;
      } else {
        currentTableFrame = nextFrame;
      }
    }

    // Restore page from swap if necessary
    if (needsRestore) {
      PMrestore(currentTableFrame, pageNumber);
    }

    return currentTableFrame;
  }
};

// Global address resolver instance
static AddressResolver g_resolver;

// Virtual Memory API Implementation
void VMinitialize() {
  MemoryUtils::zeroOutFrame(0);
}

int VMread(uint64_t virtualAddress, word_t* value) {
  if (virtualAddress >= VIRTUAL_MEMORY_SIZE || value == nullptr) {
    return 0;
  }

  uint64_t offset = MemoryUtils::extractOffsetFromVA(virtualAddress);
  uint64_t physicalFrame = g_resolver.resolveVirtualAddress(virtualAddress);
  *value = MemoryUtils::readPhysicalWord(physicalFrame, offset);

  return 1;
}

int VMwrite(uint64_t virtualAddress, word_t value) {
  if (virtualAddress >= VIRTUAL_MEMORY_SIZE) {
    return 0;
  }

  uint64_t offset = MemoryUtils::extractOffsetFromVA(virtualAddress);
  uint64_t physicalFrame = g_resolver.resolveVirtualAddress(virtualAddress);
  MemoryUtils::writePhysicalWord(physicalFrame, offset, value);

  return 1;
}