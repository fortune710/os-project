/*
 * mem.h - Memory Management & Virtual Memory API
 * Subsystem B of MOSS
 *
 * Provides logical-to-physical address translation using paging,
 * page replacement algorithms (FIFO, LRU), and page fault tracking.
 */

#ifndef MOSS_MEM_H
#define MOSS_MEM_H

#include "moss_common.h"

/*
 * mem_init - Initialize the memory management subsystem.
 * Must be called before any other mem_ functions.
 * Returns: MOSS_SUCCESS on success.
 */
int mem_init(void);

/*
 * mem_allocate - Allocate logical pages for a process.
 * @pid:       Process ID (must exist in scheduler).
 * @num_pages: Number of logical pages to allocate.
 * Returns: MOSS_SUCCESS on success.
 *          MOSS_ERR_INVALID if pid invalid or num_pages <= 0.
 *          MOSS_ERR_FULL if page table capacity exceeded.
 */
int mem_allocate(int pid, int num_pages);

/*
 * mem_access - Access a logical address for a process.
 * Performs logical-to-physical address translation.
 * May trigger a page fault and invoke page replacement.
 * @pid:          Process ID.
 * @logical_addr: 16-bit logical address.
 * Returns: Physical address (>= 0) on success.
 *          MOSS_ERR_INVALID if address out of range.
 *          MOSS_ERR_NOT_FOUND if process has no memory allocated.
 */
int mem_access(int pid, uint16_t logical_addr);

/*
 * mem_free - Free all memory allocated to a process.
 * @pid: Process ID.
 * Returns: MOSS_SUCCESS on success, MOSS_ERR_NOT_FOUND if not allocated.
 */
int mem_free(int pid);

/*
 * mem_set_replacement - Set the page replacement algorithm.
 * @algo: Algorithm name - "FIFO" or "LRU".
 * Returns: MOSS_SUCCESS on success, MOSS_ERR_INVALID if unknown algo.
 */
int mem_set_replacement(const char *algo);

/*
 * mem_print_page_table - Print the page table for a specific process.
 * @pid: Process ID.
 */
void mem_print_page_table(int pid);

/*
 * mem_print_frames - Print the current state of physical frames.
 */
void mem_print_frames(void);

/*
 * mem_print_stats - Print memory statistics (page faults, hit rate).
 */
void mem_print_stats(void);

/*
 * mem_cleanup - Free all memory subsystem resources.
 * Should be called during system shutdown.
 */
void mem_cleanup(void);

#endif /* MOSS_MEM_H */
