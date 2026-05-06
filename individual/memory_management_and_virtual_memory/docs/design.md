# Subsystem B: Memory Management & Virtual Memory — Design Document

## Overview

This subsystem implements paged virtual memory for the MOSS simulator. It provides
logical-to-physical address translation, demand paging, and two page replacement
algorithms: FIFO (First-In, First-Out) and LRU (Least Recently Used).

## Design Decisions

### Address Space Configuration

I chose a 16-bit logical address space (64 KB) with 256-byte pages. This gives up to
256 logical pages per process, which is large enough to demonstrate replacement while
keeping the numbers manageable. Physical memory has 8 frames, which is deliberately
small to ensure page replacement occurs frequently during demonstration.

The address decomposition is:
- Page number = logical_address / 256 (upper 8 bits)
- Offset = logical_address % 256 (lower 8 bits)
- Physical address = frame_number * 256 + offset

### Demand Paging

Pages are not loaded into physical memory at allocation time. When `mem_allocate()` is
called, the page table entries are created but marked invalid. Physical frames are only
assigned when a page is first accessed via `mem_access()`, triggering a page fault. This
mirrors how real operating systems conserve physical memory.

### FIFO Replacement

Each frame tracks a `load_order` counter that is set when the page is loaded. When
eviction is needed, the frame with the smallest `load_order` is selected as the victim.
This is simple but subject to Belady's anomaly.

### LRU Replacement

Each frame tracks a `last_access` timestamp updated on every access (both faults and
hits). The frame with the smallest `last_access` is the least recently used and is
evicted first. LRU generally outperforms FIFO by exploiting temporal locality.

### Per-Process Memory

Each process gets its own page table stored in a `ProcessMemory` structure. The memory
subsystem supports up to 32 concurrent processes. Frames are shared globally — any
process can be allocated any free frame, and eviction can evict any process's page.

## Assumptions

- Logical addresses are 16-bit unsigned integers (0x0000 to 0xFFFF).
- Each process can have at most 64 pages.
- Physical memory has exactly 8 frames.
- Page replacement algorithms are globally applied (not per-process).

## Limitations

- No Optimal (Belady's) replacement algorithm for theoretical comparison.
- No TLB (Translation Lookaside Buffer) simulation.
- No support for shared pages between processes.
- No disk I/O latency simulation on page faults.

## API Summary

| Function | Purpose |
|----------|---------|
| `mem_init()` | Initialize the memory subsystem |
| `mem_allocate()` | Allocate pages for a process |
| `mem_access()` | Access a logical address (translate to physical) |
| `mem_free()` | Free all memory for a process |
| `mem_set_replacement()` | Switch between FIFO and LRU |
| `mem_print_page_table()` | Display a process's page table |
| `mem_print_frames()` | Display physical frame state |
| `mem_print_stats()` | Display fault/hit statistics |
| `mem_cleanup()` | Free all resources |
