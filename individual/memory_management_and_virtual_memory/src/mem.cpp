/*
 * mem.cpp - Memory Management & Virtual Memory Implementation
 * Subsystem B of MOSS
 *
 * Implements paging with logical-to-physical address translation,
 * page replacement algorithms (FIFO, LRU), and page fault tracking.
 */

#include "moss_mem.h"
#include <string>

/* ============================================================
 * Internal Data Structures
 * ============================================================ */

/* Page table entry */
struct PageTableEntry {
    int frame_number;  /* Physical frame number, -1 if not loaded */
    int valid;         /* 1 = page is in a physical frame */
};

/* Per-process memory info */
struct ProcessMemory {
    int pid;
    int active;
    int num_pages;     /* Number of logical pages allocated */
    PageTableEntry page_table[MAX_PAGES];
};

/* Physical frame */
struct Frame {
    int occupied;      /* 1 = frame is in use */
    int pid;           /* Process owning this frame */
    int page_number;   /* Which page of the process */
    int load_order;    /* For FIFO: order in which frame was loaded */
    int last_access;   /* For LRU: timestamp of last access */
};

/* Page replacement algorithm */
enum ReplacementAlgo {
    REPLACE_FIFO,
    REPLACE_LRU
};

/* ============================================================
 * Internal State
 * ============================================================ */

static ProcessMemory proc_mem[MAX_PROCESSES];
static Frame frames[MAX_FRAMES];
static ReplacementAlgo current_algo = REPLACE_FIFO;
static int load_counter = 0;    /* Monotonic counter for FIFO ordering */
static int access_counter = 0;  /* Monotonic counter for LRU timestamps */

/* Statistics */
static int total_accesses = 0;
static int total_page_faults = 0;

/* ============================================================
 * Internal Helpers
 * ============================================================ */

/* Find a free frame. Returns frame index or -1 if none available. */
static int find_free_frame(void) {
    for (int i = 0; i < MAX_FRAMES; i++) {
        if (!frames[i].occupied) {
            return i;
        }
    }
    return -1;
}

/* Evict a page using FIFO: evict the frame loaded earliest */
static int evict_fifo(void) {
    int victim = 0;
    int min_order = frames[0].load_order;

    for (int i = 1; i < MAX_FRAMES; i++) {
        if (frames[i].occupied && frames[i].load_order < min_order) {
            min_order = frames[i].load_order;
            victim = i;
        }
    }

    /* Invalidate the evicted page in its owner's page table */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_mem[i].active && proc_mem[i].pid == frames[victim].pid) {
            int pg = frames[victim].page_number;
            if (pg >= 0 && pg < proc_mem[i].num_pages) {
                proc_mem[i].page_table[pg].valid = 0;
                proc_mem[i].page_table[pg].frame_number = -1;
            }
            break;
        }
    }

    moss_log(LOG_INFO, "FIFO eviction: Frame %d (PID=%d, Page=%d)",
             victim, frames[victim].pid, frames[victim].page_number);

    return victim;
}

/* Evict a page using LRU: evict the frame accessed least recently */
static int evict_lru(void) {
    int victim = 0;
    int min_access = frames[0].last_access;

    for (int i = 1; i < MAX_FRAMES; i++) {
        if (frames[i].occupied && frames[i].last_access < min_access) {
            min_access = frames[i].last_access;
            victim = i;
        }
    }

    /* Invalidate the evicted page in its owner's page table */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_mem[i].active && proc_mem[i].pid == frames[victim].pid) {
            int pg = frames[victim].page_number;
            if (pg >= 0 && pg < proc_mem[i].num_pages) {
                proc_mem[i].page_table[pg].valid = 0;
                proc_mem[i].page_table[pg].frame_number = -1;
            }
            break;
        }
    }

    moss_log(LOG_INFO, "LRU eviction: Frame %d (PID=%d, Page=%d)",
             victim, frames[victim].pid, frames[victim].page_number);

    return victim;
}

/* Find the process memory entry for a PID */
static ProcessMemory* find_proc_mem(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (proc_mem[i].active && proc_mem[i].pid == pid) {
            return &proc_mem[i];
        }
    }
    return nullptr;
}

/* ============================================================
 * API Implementation
 * ============================================================ */

int mem_init(void) {
    std::memset(proc_mem, 0, sizeof(proc_mem));
    std::memset(frames, 0, sizeof(frames));

    for (int i = 0; i < MAX_PROCESSES; i++) {
        proc_mem[i].active = 0;
        proc_mem[i].pid = -1;
    }

    for (int i = 0; i < MAX_FRAMES; i++) {
        frames[i].occupied = 0;
        frames[i].pid = -1;
        frames[i].page_number = -1;
    }

    current_algo = REPLACE_FIFO;
    load_counter = 0;
    access_counter = 0;
    total_accesses = 0;
    total_page_faults = 0;

    moss_log(LOG_INFO, "Memory subsystem initialized (frames=%d, page_size=%d, addr_bits=%d)",
             MAX_FRAMES, PAGE_SIZE, LOGICAL_ADDR_BITS);
    return MOSS_SUCCESS;
}

int mem_allocate(int pid, int num_pages) {
    if (pid < 0 || num_pages <= 0 || num_pages > MAX_PAGES) {
        return MOSS_ERR_INVALID;
    }

    /* Check if already allocated */
    if (find_proc_mem(pid) != nullptr) {
        return MOSS_ERR_INVALID; /* Already has memory */
    }

    /* Find a free slot */
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!proc_mem[i].active) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        return MOSS_ERR_FULL;
    }

    ProcessMemory *pm = &proc_mem[slot];
    pm->pid = pid;
    pm->active = 1;
    pm->num_pages = num_pages;

    /* Initialize page table: all pages invalid (not loaded) */
    for (int i = 0; i < num_pages; i++) {
        pm->page_table[i].frame_number = -1;
        pm->page_table[i].valid = 0;
    }

    moss_log(LOG_INFO, "Memory allocated: PID=%d, Pages=%d, Address space=%d bytes",
             pid, num_pages, num_pages * PAGE_SIZE);
    return MOSS_SUCCESS;
}

int mem_access(int pid, uint16_t logical_addr) {
    ProcessMemory *pm = find_proc_mem(pid);
    if (pm == nullptr) {
        return MOSS_ERR_NOT_FOUND;
    }

    /* Calculate page number and offset */
    int page_number = logical_addr / PAGE_SIZE;
    int offset = logical_addr % PAGE_SIZE;

    /* Check if page is within allocated range */
    if (page_number >= pm->num_pages) {
        moss_log(LOG_ERROR, "Invalid memory access: PID=%d, Addr=0x%04X (page %d out of range, max=%d)",
                 pid, logical_addr, page_number, pm->num_pages);
        return MOSS_ERR_INVALID;
    }

    total_accesses++;

    if (pm->page_table[page_number].valid) {
        /* Page hit */
        int frame = pm->page_table[page_number].frame_number;
        /* TODO: update LRU tracking here */
        int physical_addr = frame * PAGE_SIZE + offset;

        moss_log(LOG_INFO, "Page HIT: PID=%d, Logical=0x%04X -> Page=%d, Frame=%d, Physical=0x%04X",
                 pid, logical_addr, page_number, frame, physical_addr);
        return physical_addr;
    }

    /* Page fault */
    total_page_faults++;
    moss_log(LOG_WARN, "PAGE FAULT: PID=%d, Logical=0x%04X, Page=%d",
             pid, logical_addr, page_number);

    /* Find or evict a frame */
    int frame = find_free_frame();
    if (frame < 0) {
        /* Need to evict */
        if (current_algo == REPLACE_FIFO) {
            frame = evict_fifo();
        } else {
            frame = evict_lru();
        }
    }

    /* Load page into frame */
    frames[frame].occupied = 1;
    frames[frame].pid = pid;
    frames[frame].page_number = page_number;
    frames[frame].load_order = load_counter++;
    frames[frame].last_access = access_counter++;

    /* Update page table */
    pm->page_table[page_number].frame_number = frame;
    pm->page_table[page_number].valid = 1;

    int physical_addr = frame * PAGE_SIZE + offset;
    moss_log(LOG_INFO, "Page loaded: Page=%d -> Frame=%d, Physical=0x%04X",
             page_number, frame, physical_addr);

    return physical_addr;
}

int mem_free(int pid) {
    ProcessMemory *pm = find_proc_mem(pid);
    if (pm == nullptr) {
        return MOSS_ERR_NOT_FOUND;
    }

    /* Free all frames owned by this process */
    for (int i = 0; i < MAX_FRAMES; i++) {
        if (frames[i].occupied && frames[i].pid == pid) {
            frames[i].occupied = 0;
            frames[i].pid = -1;
            frames[i].page_number = -1;
        }
    }

    pm->active = 0;
    pm->pid = -1;
    pm->num_pages = 0;

    moss_log(LOG_INFO, "Memory freed: PID=%d", pid);
    return MOSS_SUCCESS;
}

int mem_set_replacement(const char *algo) {
    if (algo == nullptr) {
        return MOSS_ERR_INVALID;
    }

    std::string algo_str(algo);

    if (algo_str == "FIFO") {
        current_algo = REPLACE_FIFO;
        moss_log(LOG_INFO, "Page replacement algorithm set to FIFO");
        return MOSS_SUCCESS;
    } else if (algo_str == "LRU") {
        current_algo = REPLACE_LRU;
        moss_log(LOG_INFO, "Page replacement algorithm set to LRU");
        return MOSS_SUCCESS;
    }

    return MOSS_ERR_INVALID;
}

void mem_print_page_table(int pid) {
    ProcessMemory *pm = find_proc_mem(pid);
    if (pm == nullptr) {
        std::printf("  No memory allocated for PID %d\n", pid);
        return;
    }

    std::printf("  Page Table for PID %d (%d pages):\n", pid, pm->num_pages);
    std::printf("  %-8s %-8s %-8s\n", "Page", "Frame", "Valid");
    std::printf("  %-8s %-8s %-8s\n", "----", "-----", "-----");

    for (int i = 0; i < pm->num_pages; i++) {
        if (pm->page_table[i].valid) {
            std::printf("  %-8d %-8d %-8s\n", i, pm->page_table[i].frame_number, "YES");
        } else {
            std::printf("  %-8d %-8s %-8s\n", i, "-", "NO");
        }
    }
}

void mem_print_frames(void) {
    std::printf("  Physical Frames (%d total):\n", MAX_FRAMES);
    std::printf("  %-8s %-10s %-8s %-12s %-12s\n",
           "Frame", "Status", "PID", "Page", "Algorithm");
    std::printf("  %-8s %-10s %-8s %-12s %-12s\n",
           "-----", "------", "---", "----", "---------");

    const char *algo_label = (current_algo == REPLACE_FIFO) ? "FIFO" : "LRU";

    for (int i = 0; i < MAX_FRAMES; i++) {
        if (frames[i].occupied) {
            std::printf("  %-8d %-10s %-8d %-12d %-12s\n",
                   i, "OCCUPIED", frames[i].pid, frames[i].page_number, algo_label);
        } else {
            std::printf("  %-8d %-10s %-8s %-12s %-12s\n",
                   i, "FREE", "-", "-", algo_label);
        }
    }
}

void mem_print_stats(void) {
    std::printf("  Memory Statistics:\n");
    std::printf("  %-25s %d\n", "Total accesses:", total_accesses);
    std::printf("  %-25s %d\n", "Page faults:", total_page_faults);
    std::printf("  %-25s %d\n", "Page hits:", total_accesses - total_page_faults);

    /* TODO: calculate and display hit rate / fault rate */

    std::printf("  %-25s %s\n", "Replacement algorithm:",
           (current_algo == REPLACE_FIFO) ? "FIFO" : "LRU");
    std::printf("  %-25s %d\n", "Physical frames:", MAX_FRAMES);
    std::printf("  %-25s %d bytes\n", "Page size:", PAGE_SIZE);
}

void mem_cleanup(void) {
    std::memset(proc_mem, 0, sizeof(proc_mem));
    std::memset(frames, 0, sizeof(frames));

    for (int i = 0; i < MAX_PROCESSES; i++) {
        proc_mem[i].pid = -1;
    }

    load_counter = 0;
    access_counter = 0;
    total_accesses = 0;
    total_page_faults = 0;

    moss_log(LOG_INFO, "Memory subsystem cleaned up");
}
