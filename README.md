*This project has been created as part of the 42 curriculum by dasamuel.*

# malloc

## Description

This project reimplements the C memory allocator as a shared library, `libft_malloc.so`.
It provides `malloc`, `free`, `realloc` and `show_alloc_mem`, and builds them only on top
of the `mmap` and `munmap` system calls.

Because the library exports the standard symbols, it can replace the system allocator in
any existing program through `LD_PRELOAD`. `ls`, `git`, `vim` and `python3` all run on it.

### Bonus

1. **Thread safety**: a `pthread` mutex protects `malloc`, `free`, `realloc`, `calloc`,
   `show_alloc_mem` and `show_alloc_mem_ex`.
2. **Defragmentation**: when a block is freed, it is merged with its free neighbours.
3. **`show_alloc_mem_ex()`**: lists every block, free ones included.
4. **Hexdump**: `show_alloc_mem_ex()` shows the content of each allocated block.
5. **Allocation history**: `show_alloc_mem_ex()` shows the last 64 operations.
6. **Debug environment variables**:
   - `FT_MALLOC_SCRIBBLE=1`: fills allocated memory with `0xAA` and freed memory with `0x55`;
   - `FT_MALLOC_VERBOSE=1`: logs every operation on stderr;
   - `FT_MALLOC_CHECK=1`: reports invalid `free` and `realloc` calls (`=2` also aborts).
7. **libc compatibility**: `calloc`, `reallocarray`, `malloc_usable_size`, `memalign`,
   `posix_memalign`, `aligned_alloc` and `valloc` are replaced too. No glibc pointer can
   then reach our `free`.

## Instructions

```sh
make                                         # builds libft_malloc_$HOSTTYPE.so + the libft_malloc.so link
make test                                    # runs the test suite

LD_PRELOAD=./libft_malloc.so ls -la          # run any existing program with it

cc -I includes main.c -L. -lft_malloc        # or link it in your own program
LD_LIBRARY_PATH=. ./a.out
```

`make test` runs the tests of the evaluation sheet (`test0` to `test6`, plus the page
count), then bonus tests for threads, edge cases and the debug variables.

Example output of `show_alloc_mem()`:

```
SMALL : 0x7F6F92376000
0x7F6F92376040 - 0x7F6F92376360 : 800 bytes
TINY : 0x7F6F925A0000
0x7F6F925A0040 - 0x7F6F925A006A : 42 bytes
Total : 842 bytes
```

## Algorithm and data structures

### Zones

| Type  | Size         | Storage                                           |
|-------|--------------|---------------------------------------------------|
| TINY  | ≤ 128 bytes  | 16 KB zones (4 pages), at least 100 blocks each   |
| SMALL | ≤ 1024 bytes | 104 KB zones (26 pages), at least 100 blocks each |
| LARGE | > 1024 bytes | one `mmap` per allocation                         |

Small requests are the most common. Grouping them into pre-allocated zones avoids a
system call and a wasted page for every `malloc`. Large blocks get their own mapping, which
is given back to the system as soon as they are freed.

### Blocks and free lists

Inside a zone, memory is a linked list of blocks. Each block starts with a 32-byte header
(its size, whether it is free, and the next block), and its data is aligned on 16 bytes.

Free blocks are also sorted by size into lists, one per multiple of 16 bytes. A fitting
block is found right away, without walking the zone. A block that is bigger than needed is
split in two.

### Freeing

A freed block is merged with the free blocks around it, so free memory stays in large
pieces. Empty zones are given back to the system, except one per type, which is kept to
avoid calling `mmap` and `munmap` over and over.

### Finding a pointer

All zones are kept in an array sorted by address. `free` and `realloc` find the zone of a
pointer with a binary search, then check that it is a real allocated block. Invalid
pointers are ignored instead of corrupting memory. `show_alloc_mem` uses the same array to
print the zones in address order.

`realloc` resizes the block in place when it can. Otherwise it allocates a new block and
copies the data.

### Globals

There are only two globals, as the subject allows: `g_heap` holds the allocator state and
`g_mutex` provides thread safety.

## Resources

- `man 2 mmap`, `man 2 munmap`, `man 3 malloc`, `man 3 pthread_mutex_lock`
- glibc wiki, *Malloc Internals*: <https://sourceware.org/glibc/wiki/MallocInternals>