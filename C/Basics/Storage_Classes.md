# C Storage Classes, Virtual Memory Map, Scope & Linkage

## Contents

- [Key terms explained (read this first)](#key-terms-explained-read-this-first)
- [1. Storage classes in C](#1-storage-classes-in-c)
- [2. A process's virtual memory map](#2-a-processs-virtual-memory-map)
- [3. Scope](#3-scope)
- [4. Linkage](#4-linkage)
- [5. Master table: everything in one place](#5-master-table-everything-in-one-place)
- [6. Interview questions with model answers](#6-interview-questions-with-model-answers)
- [7. Code puzzles with explanations](#7-code-puzzles-with-explanations)
- [8. The protocol-developer angle (Senior / Staff follow-ups)](#8-the-protocol-developer-angle-senior--staff-follow-ups)

## Key terms explained (read this first)

Every term used later in the notes is defined here in plain words. If a sentence further down feels dense, come back to this table.

### The core ideas

| Term | Plain-English meaning | Tiny example |
| --- | --- | --- |
| **Object** | A piece of memory that holds a value (a variable, an array, a struct). | `int x;` creates an object that holds an int. |
| **Identifier / name** | The label you write in source code to refer to an object or function. | `x`, `main`, `rx_pkts` |
| **Translation unit (TU)** | One `.c` file after the preprocessor has pasted in all its `#include`d headers. The compiler works on one TU at a time. | `stats.c` plus the headers it includes |
| **Declaration** | Tells the compiler "this name exists and has this type". No memory is reserved. | `extern int x;`  `int f(void);` |
| **Definition** | A declaration that also reserves memory (for a variable) or supplies the body (for a function). | `int x;`  `int f(void){ return 1; }` |
| **Storage class specifier** | A keyword (`auto`, `register`, `static`, `extern`, `_Thread_local`) that changes lifetime, visibility or linkage of what you declare. | `static int count;` |
| **Storage duration (lifetime)** | *How long the object exists at run time.* | See the four kinds below. |
| **Scope** | *Where in the source code the name can be used.* A compile-time idea about names. | A local variable is usable only inside its `{ }`. |
| **Linkage** | *Whether the same name in different `.c` files refers to the same thing.* The linker decides this. | A global `int x;` in two files is one shared variable. |
| **Symbol** | The name of a function or global variable as stored in the compiled `.o` file, which the linker matches up. | `nm a.out` lists symbols. |
| **Linker** | The tool that joins all `.o` files and libraries into one program and connects each use of a symbol to its definition. | `ld`, run by `gcc` |

### The four storage durations

| Duration | Object exists... | Typical example |
| --- | --- | --- |
| **Automatic** | From entering the block to leaving it. Created and destroyed for you. | Ordinary local variables |
| **Static** | For the whole run of the program. Set up before `main` starts. | Globals, `static` locals |
| **Thread** | For the whole life of one thread; each thread has its own copy. | `_Thread_local` variables |
| **Allocated** | From `malloc` until you call `free`. You control it. | `int *p = malloc(64);` |

### The three kinds of linkage

| Linkage | Meaning | How you get it |
| --- | --- | --- |
| **External** | One shared entity visible to every `.c` file. | A global variable or function with no `static` |
| **Internal** | Shared only inside one `.c` file; hidden from the others. | A global marked `static` |
| **None** | Not shared at all; each declaration is its own separate thing. | Local variables, parameters |

### Values and behaviour words

| Term | Plain-English meaning |
| --- | --- |
| **Indeterminate value** | "Whatever garbage happened to be in that memory." Local variables start this way until you assign them. Reading one is a bug. |
| **Zero-initialised** | Set to 0 (or null pointer, 0.0) automatically before the program starts. Applies to globals and `static` variables. |
| **Undefined behaviour (UB)** | The C standard puts no rules on what happens. The program may crash, print garbage, or seem to work today and fail tomorrow. Examples: reading an uninitialised local, writing to a string literal, using a dangling pointer. |
| **Constant expression** | A value the compiler can compute while compiling, such as `5` or `2 * 8`. A normal variable, even a `const` one, is not one in C. Initialisers of globals and `static` variables must be constant expressions. |
| **Tentative definition** | A file-scope `int x;` with no initialiser. It acts as a definition if no other definition appears; older compilers merged duplicates across files, newer GCC (10+) reports an error. |
| **Dangling pointer** | A pointer to memory that no longer holds a live object, for example the address of a local variable after its function returned. |
| **Shadowing** | An inner variable with the same name hides an outer one inside the inner block. |

### Memory-map words

| Term | Plain-English meaning |
| --- | --- |
| **Virtual memory** | Each process sees its own private block of addresses. The CPU's MMU and the OS translate those virtual addresses to real RAM behind the scenes. Two processes can both use address `0x1000` without clashing. |
| **MMU / page table** | The hardware and the OS data structure that perform and record that translation. |
| **Page** | The unit of memory the OS manages, usually 4 KiB. Permissions (read, write, execute) are set per page. |
| **Demand paging** | A page gets real RAM only when the program first touches it. |
| **Segment / section** | A named region of the program image or address space. `.text`, `.rodata`, `.data` and `.bss` are sections of the executable file. |
| **`.text`** | The machine code. Read and execute, not write. |
| **`.rodata`** | Read-only data: string literals and `const` globals. |
| **`.data`** | Global and `static` variables that start with a non-zero value. Read and write. Stored in the file. |
| **`.bss`** | Global and `static` variables that start at zero. The file stores only their total size, so they cost no file space. |
| **Stack** | Memory for local variables, function arguments and return addresses. Grows downward. Each call adds a *frame*; returning removes it. Fast, but small. |
| **Heap** | Memory you request with `malloc` and release with `free`. Grows upward from the end of `.bss`. |
| **`mmap` region** | Area between heap and stack where shared libraries, memory-mapped files, large `malloc` blocks and thread stacks are placed. |
| **Program break (`brk`)** | The current end of the heap. `malloc` moves it with `brk`/`sbrk` for small requests. |
| **Guard page** | An unmapped page placed beside a stack so that overflowing it causes an immediate crash instead of silent corruption. |
| **Copy-on-write (COW)** | After `fork()`, parent and child share the same physical pages. A page is copied only when one of them writes to it. |
| **ASLR** | Address Space Layout Randomisation: the OS puts the stack, heap and libraries at random addresses on each run, making attacks harder. |
| **PIE** | Position-Independent Executable: a program built so it can be loaded at any address, which allows ASLR to move the program itself. |
| **W^X / NX** | A page may be writable or executable but not both, which blocks running injected code. |
| **ELF** | The executable and object-file format used on Linux. |
| **PLT / GOT** | Tables used for calling functions in shared libraries. The PLT stub jumps through a GOT entry, which the dynamic linker fills in with the real address. |
| **TLS** | Thread-Local Storage: the area that holds each thread's private copy of `_Thread_local` variables. |

### Concurrency and systems words

| Term | Plain-English meaning |
| --- | --- |
| **Reentrant** | A function that is safe to call again before an earlier call has finished (from a signal handler or another thread). It uses no shared static state. |
| **Thread-safe** | Safe to call from several threads at once. A function using a `static` buffer is usually not. |
| **Race condition** | A bug where the result depends on the timing of two threads touching the same data. |
| **Atomic** | An operation that cannot be seen half-done by another thread (C11 `<stdatomic.h>`). |
| **`volatile`** | Tells the compiler "this value can change outside your view (hardware, signal handler); do not skip or reorder reads and writes". It does **not** make code thread-safe. |
| **Async-signal-safe** | A function that may be called from inside a signal handler, such as `write`. `malloc` and `printf` are not. |
| **Cache line** | The unit (usually 64 bytes) in which the CPU moves memory in and out of its cache. |
| **False sharing** | Two threads write different variables that sit on the same cache line, so the line bounces between cores and slows both. |
| **Hugepage** | A large memory page (2 MiB or 1 GiB instead of 4 KiB) that reduces TLB misses for big tables. |
| **TLB** | A small cache inside the CPU of recent address translations. A miss costs extra time. |
| **NUMA** | Multi-socket machines where memory attached to one CPU is faster for that CPU than memory on another. |
| **RSS** | Resident Set Size: how much real RAM a process is using right now. |
| **RCU** | Read-Copy-Update: a technique that lets many readers use a table without locks while a writer swaps in a new version. |

### Linker and library words

| Term | Plain-English meaning |
| --- | --- |
| **Strong / weak symbol** | A normal definition is strong. A `weak` one is a default that a strong definition of the same name silently replaces. |
| **Symbol interposition** | Another library or `LD_PRELOAD` supplies a function of the same name that gets called instead of the original. |
| **Symbol visibility** | Whether a symbol in a shared library (`.so`) is exported to outsiders (`default`) or kept internal (`hidden`). |
| **Shared library (`.so`)** | Code loaded at run time and shared between programs. |
| **`dlopen`** | Loads a shared library while the program is running, used for plugins. |
| **`extern "C"`** | C++ keyword that stops name mangling so C++ code can link with C symbols. |
| **Inline function** | A function the compiler may expand in place of the call to save call overhead. |

## 1. Storage classes in C

A storage-class specifier tells the compiler three things about an object: how long it lives (storage duration), who can name it (scope), and whether other translation units can refer to it (linkage). C11 defines five: `auto`, `register`, `static`, `extern` and `_Thread_local`. A declaration may carry only one, except that `_Thread_local` can be combined with `static` or `extern`. `typedef` is syntactically a storage-class specifier but is not a storage class.

Four storage durations exist: **automatic** (block entry to exit), **static** (whole program), **thread** (whole thread) and **allocated** (malloc to free). Allocated is not selected by any keyword.

| Specifier | Duration | Scope | Linkage | Where it usually lives | Default value |
| --- | --- | --- | --- | --- | --- |
| `auto` (local default) | automatic | block | none | stack or a register | indeterminate |
| `register` | automatic | block | none | a CPU register if the compiler agrees (only a hint); you may not use `&` on it | indeterminate |
| `static` inside a function | static | block | none | `.data` / `.bss` | zero |
| `static` at file scope | static | file | internal | `.data` / `.bss` | zero |
| `extern` | static | where declared | external (normally) | `.data` / `.bss` of the defining file | zero |
| `_Thread_local` | thread | as declared | as declared | TLS (`.tdata` / `.tbss`) | zero |

Placement in the table is how real toolchains behave, not something the standard mandates. The standard only guarantees duration, scope and linkage.

### How to read each row in plain words

| Row | What it means |
| --- | --- |
| `auto` | A normal local variable. Created when its block starts, destroyed when it ends, visible only inside that block, starts with garbage. |
| `register` | A local variable where you ask the compiler to keep it in a fast CPU register. The compiler is free to ignore the request. The one rule it always enforces: you cannot take its address with `&`, because a register has no memory address. Otherwise it behaves exactly like `auto`. |
| `static` inside a function | A local name whose value is remembered between calls. It is created once before `main`, lives until the program ends, and starts at 0. Only that function can name it. |
| `static` at file scope | A global that is private to its `.c` file. Other files cannot see it, even if they declare the same name. |
| `extern` | "This variable is defined in another file." It reserves no memory; it only lets this file use the name. |
| `_Thread_local` | Like a global, but every thread gets its own private copy. |

Column meanings: **Duration** is how long it exists, **Scope** is where its name works, **Linkage** is whether other files can share it, **Where it usually lives** is the memory region (see section 2), and **Default value** is what it holds before you assign anything.

### auto

Every local variable is `auto` unless told otherwise, so the keyword is almost never written. The value is indeterminate until assigned, and reading it is a bug.

```c
void f(void) {
    int x;          // auto, garbage value
    auto int y = 5; // legal, redundant
    printf("%d\n", x); // BUG: uninitialised read
}
```

Trap: in C23, `auto` also means type inference (`auto n = 5;` gives `int`), as in C++11. Say which standard you mean.

### register

A hint to keep the object in a register. Its one hard rule is that you cannot take its address.

```c
register int i = 0;
int *p = &i;   // ERROR: address of register variable requested
```

Modern compilers allocate registers better than humans and ignore the hint, but the `&` restriction is still enforced. C++17 removed the keyword.

### static (two meanings)

This is the most-asked point: `static` means different things depending on where it appears.

- **Inside a function**: the object has static duration. It is initialised once and keeps its value between calls. Scope stays local to the block.
- **At file scope** (variable or function): the name gets internal linkage, so it is invisible to other `.c` files. Duration was already static.

```c
int counter(void) {
    static int n = 0;  // initialised once, before main; lives in .bss
    return ++n;        // 1, 2, 3, ... across calls
}

static int hits;           // file scope: internal linkage, zero-initialised
static void helper(void);  // function with internal linkage
```

Rules to quote: the initialiser of a static object must be a constant expression in C (not so in C++), and a static local is **not reentrant and not thread-safe**. That is why `strtok()` and `inet_ntoa()`, which return pointers into static buffers, are replaced by `strtok_r()` and `inet_ntop()` in protocol code.

### extern

`extern` declares an object defined elsewhere. It allocates nothing and says only "this name exists, the linker will find it".

```c
// config.c
int mtu = 1500;          // definition: allocates storage, external linkage

// rx.c
extern int mtu;          // declaration: no storage
void rx(void) { use(mtu); }
```

Two details interviewers probe. First, `extern int x = 5;` is a definition, not a declaration (compilers warn). Second, `int x;` at file scope is a *tentative definition*: GCC before version 10 merged duplicates across files (`-fcommon`), but since GCC 10 the default is `-fno-common`, so two files each writing `int x;` now fail with "multiple definition".

### \_Thread\_local

Gives each thread its own copy. Duration is the thread's lifetime, and the object is initialised when the thread starts.

```c
#include <threads.h>               // thread_local macro (keyword in C23)
static thread_local int err_code;  // one copy per thread
// GCC/Clang extension: static __thread int err_code;
```

`errno` is the classic example. The linker places such variables in `.tdata` (initialised) and `.tbss` (zero), and each new thread gets a copy; on x86-64 Linux access is a `%fs`-relative load.

## 2. A process's virtual memory map

Every process sees its own private, contiguous virtual address space; the MMU and page tables translate it to physical frames on demand. The layout below is x86-64 Linux with a 48-bit address space, top-down mmap layout and ASLR on. Exact addresses differ per architecture, but the order of regions is the interview answer.

```
High addresses
0xFFFFFFFFFFFFFFFF  +--------------------------------+
                    |  Kernel space (upper half)     |  not accessible from user mode
0xFFFF800000000000  +--------------------------------+
                    |  Non-canonical hole            |  any access faults
0x00007FFFFFFFFFFF  +--------------------------------+  <- top of user space (128 TiB)
                    |  argv[], envp[], auxv          |
                    +--------------------------------+
                    |  STACK                         |  automatic vars, return addrs,
                    |        |                       |  saved regs; grows DOWN
                    |        v                       |  (guard page below)
                    |         (large free gap)       |
                    |  MEMORY-MAPPED REGION          |  shared libs (libc.so), mmap(),
                    |        |                       |  big malloc, thread stacks,
                    |        v  (top-down layout)    |  hugepages, shm
                    |                                |
                    |         (large free gap)       |
                    |        ^                       |
                    |        |                       |
                    |  HEAP (brk / sbrk)             |  small malloc/new; grows UP
                    +--------------------------------+  <- program break
                    |  .bss     (zero-initialised)   |  not stored in the file
                    +--------------------------------+
                    |  .data    (initialised, RW)    |
                    +--------------------------------+
                    |  .rodata  (consts, literals,R) |
                    +--------------------------------+
                    |  .text    (code, R-X)          |
0x0000000000400000  +--------------------------------+  non-PIE base (PIE: random ~0x55..)
                    |  Unmapped                      |  NULL dereference -> SIGSEGV
0x0000000000000000  +--------------------------------+
Low addresses
```

With the modern top-down layout, the stack grows down from the top of user space, and the mmap region also grows downward, starting just below the stack. The heap grows up from the end of `.bss`. So the heap and the mmap region head toward each other, with a gap of many terabytes between them in a 64-bit space; they never meet in practice. (In the older bottom-up layout, mmap started low and grew up.)

### Segment by segment

| Region | Permissions | Contains | In the ELF file? |
| --- | --- | --- | --- |
| `.text` | r-x | machine code, PLT stubs | yes |
| `.rodata` | r-- | string literals, `const` globals, jump tables | yes |
| `.data` | rw- | initialised globals and statics (non-zero) | yes |
| `.bss` | rw- | uninitialised or zero-initialised globals and statics | no, only its size is recorded |
| Heap | rw- | `malloc` blocks (below the mmap threshold) | no |
| mmap region | varies | shared libraries, anonymous maps, file maps, large `malloc`, thread stacks | no |
| Stack | rw- | locals, arguments, return address, saved frame pointer | no |

### Where does each variable go?

```c
#include <stdio.h>
#include <stdlib.h>

int g_init = 10;          // .data
int g_zero;               // .bss
const int g_const = 5;    // .rodata
static int s_file;        // .bss, internal linkage
static int s_init = 3;    // .data, internal linkage
char *msg = "hello";      // msg itself: .data;  the characters "hello": .rodata
char buf[1024];           // .bss

int main(void) {
    int a = 1;            // stack
    static int s_loc = 7; // .data (static duration, block scope)
    static int s_zero;    // .bss
    char arr[] = "hi";    // stack: 3-byte copy of the literal
    char *p = "hi";       // p on stack; characters in .rodata
    int *h = malloc(64);  // h on stack; the 64 bytes on the heap
    p[0] = 'H';           // UB: writing to .rodata -> SIGSEGV
    free(h);
}
```

Points that get probed:

- A zero-initialised global (`int x = 0;`) goes to `.bss`, not `.data`, so the binary stays small.
- `char arr[] = "hi"` copies the literal onto the stack and is writable; `char *p = "hi"` points into read-only memory.
- A `const` local is still a stack object (the compiler may fold it away); only file-scope or static `const` objects reach `.rodata`.
- The pointer `h` and the block it points to live in two different regions.

### Why the segments exist

- **Permissions**: code is r-x and data is rw- with no execute (W^X / NX), which stops injected-code attacks.
- **Sharing**: `.text` and `.rodata` of the same binary or library are mapped once in physical memory and shared by all processes.
- **Lazy allocation**: `.bss` and the heap are backed by zero pages only when first touched (demand paging).
- **fork()**: the child gets the same mappings, marked copy-on-write; pages are duplicated only on the first write.

### malloc and the heap

glibc serves small requests from the brk heap and requests above `M_MMAP_THRESHOLD` (128 KiB by default, adjusted dynamically) from a private `mmap`. Large blocks therefore return memory to the OS on `free`, while small ones may stay in the arena and fragment. Threads use separate arenas.

### Inspect it yourself

```
$ gcc -O0 -no-pie -o a.out a.c
$ size a.out                  # text, data, bss sizes
$ readelf -S a.out            # section table
$ nm -n a.out | head          # symbols in address order (T text, D data, B bss, R rodata)
$ cat /proc/<pid>/maps        # live regions, permissions, backing files
```

Illustrative `/proc/<pid>/maps` shape (addresses vary):

```
00400000-00401000 r--p  /home/u/a.out
00401000-00402000 r-xp  /home/u/a.out        <- .text
00404000-00405000 rw-p  /home/u/a.out        <- .data/.bss
01a3c000-01a5d000 rw-p  [heap]
7f3c1a000000-7f3c1a1c5000 r-xp  /lib/x86_64-linux-gnu/libc.so.6
7ffd8e3f0000-7ffd8e411000 rw-p  [stack]
```

On 32-bit Linux the same picture applies with a 3 GiB user / 1 GiB kernel split.

## 3. Scope

Scope is the region of source text where an identifier is visible. It is a compile-time property of a *name*; it is different from lifetime, which is a run-time property of an *object*, and from linkage, which is about the linker. C has four kinds of scope.

| Scope | Starts and ends | Example |
| --- | --- | --- |
| Block | from the declaration to the closing `}` | locals, loop variables (`for (int i...)`) |
| Function | the whole function body | only `goto` labels |
| File | from the declaration to the end of the translation unit | globals, function names |
| Function prototype | inside the parameter list only | `void f(int n, int a[n]);` |

### Block scope and shadowing

An inner declaration hides an outer one with the same name until the inner block ends.

```c
int x = 1;                 // file scope

int main(void) {
    int x = 2;             // block scope, shadows the global
    {
        int x = 3;         // shadows again
        printf("%d\n", x); // 3
    }
    printf("%d\n", x);     // 2
    // the global x is hidden here; C has no :: operator (see puzzle P2)
}
```

C has no `::` operator. A shadowed global can still be reached from a nested block that has not declared its own `x`, by writing `extern int x;` there (see puzzle P2), but the better fix is to rename. Compile with `-Wshadow` to catch accidental shadowing. Declaring the same name twice in one block (`int x; int x;`) is an error; declaring it again in a nested block is fine.

### Scope of the declaration point

A name's scope begins just after its declarator, so an object can refer to itself in its own initialiser:

```c
int x = 5;
void f(void) {
    int x = x + 1;   // inner x is already in scope: reads an indeterminate value
}
```

### Function scope (labels)

Labels are visible throughout their function, even before the label appears, so `goto` can jump forward. They live in their own namespace, so a label and a variable may share a name.

### File scope

A declaration outside any function is visible from that point to the end of the file, so order matters:

```c
void g(void) { h(); }   // h not declared yet: "implicit declaration" (C89 allowed it; C99+ warns, newer GCC/Clang reject it)
void h(void) { }
```

This is why prototypes live in headers.

### Prototype scope

Parameter names in a declaration (not a definition) are visible only inside the parentheses, which is what lets `void f(int n, int a[n]);` use `n` for the array size.

### Scope, lifetime and linkage are independent

```c
void f(void) {
    static int n;     // scope: block.  lifetime: whole program.  linkage: none
}
static int g;         // scope: file.   lifetime: whole program.  linkage: internal
int h;                // scope: file.   lifetime: whole program.  linkage: external
```

A classic bug that shows the difference: returning the address of a local.

```c
int *bad(void) { int v = 42; return &v; }   // v's lifetime ended; dangling pointer
int *ok(void)  { static int v = 42; return &v; }  // lifetime is the whole program
```

The name `v` is out of scope outside the function in both cases, but only the `static` object still exists after the return.

### Four name spaces

Identifiers fall into separate name spaces, so these all coexist: labels; tags (`struct`, `union`, `enum`); members of each struct or union; and ordinary identifiers (variables, functions, typedef names, enumerators).

```c
struct stat { int st_size; };   // tag
int stat(const char *path, struct stat *buf);  // ordinary identifier - same spelling, no clash
```

## 4. Linkage

Linkage decides whether two declarations of the same identifier, in the same or different translation units (TUs), refer to the **same object or function**. A TU is one `.c` file after preprocessing. The compiler emits symbols; the linker resolves them, so linkage is what the linker sees.

| Linkage | Meaning | How you get it |
| --- | --- | --- |
| External | one entity shared across all TUs | file-scope variable or function with no `static`; `extern` declarations |
| Internal | one entity shared across one TU only | file-scope `static`; file-scope `const` in C++ (not in C) |
| None | each declaration is a distinct entity | locals, parameters, `typedef` names, struct tags, enumerators |

### How the rules resolve

```c
int a;               // file scope, no specifier  -> external linkage
static int b;        // internal linkage
extern int c;        // external (declares; defined elsewhere)
static int d;
extern int d;        // refers to the earlier visible declaration -> stays INTERNAL

void f(void) {
    int x;           // none
    static int y;    // none (static duration, but no linkage)
    extern int a;    // external: refers to the global a above
}
```

The key rule: an `extern` declaration takes the linkage of a prior visible declaration; with none visible it is external. Functions follow the same rules, and `extern` is implicit on function declarations (`int f(void);` equals `extern int f(void);`).

### Multi-file example

```c
// stats.h
extern unsigned long rx_pkts;      // declaration only
void stats_bump(void);

// stats.c
#include "stats.h"
unsigned long rx_pkts;             // the ONE definition (external)
static unsigned long drops;        // private to stats.c (internal)
static void log_drop(void) { drops++; }   // private helper
void stats_bump(void) { rx_pkts++; }      // exported

// main.c
#include "stats.h"
int main(void) {
    stats_bump();
    rx_pkts += 10;                 // same object as in stats.c
    // drops = 0;                  // error: undeclared here; and a fresh 'drops' in
}                                  // another file would be a DIFFERENT variable
```

```
  stats.o                      main.o
  --------------------------   ----------------------
  GLOBAL  rx_pkts   (B)  <-----  UNDEF rx_pkts
  GLOBAL  stats_bump (T) <-----  UNDEF stats_bump
  LOCAL   drops     (b)         (static: not visible to the linker's
  LOCAL   log_drop  (t)          cross-file resolution)
```

In `nm` output, uppercase letters are external symbols and lowercase letters are internal (static) ones.

### Common errors and what they mean

| Symptom | Cause | Fix |
| --- | --- | --- |
| `undefined reference to 'x'` | declared (`extern`) but never defined, or the defining `.o` / library is missing from the link line | add the definition or the object |
| `multiple definition of 'x'` | defined in two TUs, often `int x;` in a header that two `.c` files include (an error by default since GCC 10, because of `-fno-common`) | `extern` in the header, one definition in a `.c` |
| `static` variable in a header | no error, but every including TU gets its own private copy, so state is silently not shared | declare `extern` in the header |
| Two TUs each define `static int x` | legal: two unrelated variables | none needed |

### Headers: what belongs where

- **Header**: `extern` declarations, function prototypes, types, macros, `static inline` functions.
- **Source**: definitions of globals and non-inline functions.

An `inline` function in C needs care. A plain `inline int f(void){...}` in a header does not by itself provide an external definition; exactly one TU must add `extern inline int f(void);` (C99 semantics). The safe, common pattern is `static inline` in the header, which gives each TU its own internal copy.

### Linkage and shared libraries (symbol visibility)

External linkage means "visible to the static linker", but in an ELF shared object it also means "exported to everyone" by default, which pollutes the dynamic symbol table, slows loading and allows accidental symbol interposition.

```c
#define API __attribute__((visibility("default")))

API int proto_init(void);          // exported
int proto_internal_helper(void);   // with -fvisibility=hidden: not exported, yet
                                   // still callable between files of the same .so
```

Build with `-fvisibility=hidden` and export only the intended API, or use a version script. Prefer `static` for anything used by one file. The three levels, from narrowest to widest, are `static` (file only), hidden visibility (inside the `.so`) and default visibility (public ABI).

### Weak symbols and interposition

A `__attribute__((weak))` definition has external linkage but yields to a strong definition of the same name at link time. Libraries use it for overridable defaults (for example a platform hook). By default, a function in a shared library can be overridden by an earlier library or `LD_PRELOAD` (symbol interposition), which is how `malloc` wrappers work; `-Bsymbolic` or hidden visibility prevents this.

## 5. Master table: everything in one place

| Declaration | Scope | Lifetime | Linkage | Segment | Default value |
| --- | --- | --- | --- | --- | --- |
| `int x;` inside a function | block | automatic | none | stack | indeterminate |
| `register int x;` inside a function | block | automatic | none | register or stack | indeterminate |
| `static int x;` inside a function | block | static | none | `.bss` | 0 |
| `static int x = 5;` inside a function | block | static | none | `.data` | 5 |
| `int x;` at file scope | file | static | external | `.bss` | 0 |
| `int x = 5;` at file scope | file | static | external | `.data` | 5 |
| `static int x;` at file scope | file | static | internal | `.bss` | 0 |
| `const int x = 5;` at file scope | file | static | external (C) | `.rodata` | 5 |
| `extern int x;` | where written | static | external (or inherited) | defining TU | n/a, declaration only |
| `_Thread_local int x;` | as declared | thread | as declared | `.tbss` | 0 |
| `malloc(n)` | via pointer | until `free` | none | heap or mmap | indeterminate |
| function parameter | function body | automatic | none | register or stack | the argument value |
| string literal `"abc"` | n/a | static | none | `.rodata` | its characters |

Rules of thumb to say out loud:

- **Lifetime** is set by *where* and whether `static` appears. **Linkage** is set by whether the name is at file scope and whether it carries `static`. **Scope** is set by *where* it is written.
- `static` at file scope shrinks linkage; `static` in a block extends lifetime. Neither one changes the other property.
- Only automatic objects are uninitialised by default; everything static or thread-local starts as zero (pointers become null, floats become 0.0).
- File-scope `const` has external linkage in C but internal linkage in C++, which matters when headers are shared between the two.

## 6. Interview questions with model answers

### Fundamentals

**Q1. What is the difference between a declaration and a definition?**

A declaration introduces a name and its type; a definition also allocates storage (objects) or provides the body (functions). `extern int x;` and `int f(void);` are declarations; `int x;` and `int f(void){...}` are definitions. An object may be declared many times but defined exactly once across the whole program if it has external linkage and is used.

**Q2. What are the two meanings of `static`?**

At file scope it gives internal linkage (hides the name from other TUs). Inside a block it gives static storage duration (value persists across calls, initialised once). In a function parameter, `int a[static 10]` (C99) means the pointer must reference at least 10 valid elements, a third and rarely used meaning.

**Q3. Scope vs lifetime vs linkage?**

Scope is where the name is visible (compile time). Lifetime is when the object exists (run time). Linkage is whether the same name in another TU is the same entity (link time). A `static` local has block scope, program lifetime and no linkage, which shows that the three are independent.

**Q4. Where does a `static` local variable live, and when is it initialised?**

In `.data` if it has a non-zero initialiser, otherwise `.bss`. Because C requires a constant initialiser, the value is baked into the image at load time, so there is no run-time check; C++11 differs, since a function-local static with a dynamic initialiser is guarded and thread-safe.

**Q5. Can you take the address of a `register` variable?**

No, it is a constraint violation, so the compiler must diagnose it. That guarantee is the only thing `register` still enforces.

**Q6. Difference between `extern int x;` and `int x;` at file scope?**

The first never allocates. The second is a tentative definition; if no initialised definition appears in the TU it becomes a zero-initialised definition. With `-fno-common` (GCC 10+ default) two TUs both defining `int x;` produce a *multiple definition* error.

**Q7. `extern int d; static int d;` compiles?**

No. A `static` declaration after a non-static one for the same identifier is an error (linkage conflict). The reverse order is accepted and the name keeps internal linkage.

### Memory layout

**Q8. Difference between `.data`, `.bss` and `.rodata`?**

`.data` holds initialised writable globals and statics and takes space in the file. `.bss` holds zero-initialised ones; only its size is recorded, and the kernel supplies zero pages. `.rodata` is read-only data (constants, string literals) and is mapped without write permission. The `size` command reports `text` (code + rodata), `data` and `bss`.

**Q9. `char *p = "abc"; p[0] = 'x';` versus `char a[] = "abc"; a[0] = 'x';`?**

The first writes into a string literal in `.rodata`: undefined behaviour, typically SIGSEGV. The second copies the literal into a writable array (stack for a local, `.data` for a global), so it is fine. Declare the first as `const char *p`.

**Q10. Stack vs heap?**

Stack: automatic, allocated by moving the stack pointer (a few instructions), freed on return, small (8 MiB default main-thread limit, set by `ulimit -s`), thread-private. Heap: explicit lifetime via `malloc`/`free`, slower, can fragment, shared by all threads, size limited by address space and RAM. Stack overflow hits a guard page and raises SIGSEGV.

**Q11. Does uninitialised data cost space in the executable?**

`.bss` does not (only a size in the program header); `.data` does. That is why a 1 MiB zero-initialised table is free on flash, while `int t[1024*256] = {1};` bloats the image.

**Q12. What does the heap grow with, and what is the mmap threshold?**

`brk`/`sbrk` moves the program break for the main arena; glibc uses `mmap` for allocations above `M_MMAP_THRESHOLD` (128 KiB default, dynamic) and for secondary thread arenas. Freed mmap chunks go straight back to the OS.

**Q13. What happens to memory on `fork()`?**

The child gets a copy of the page tables with every writable page marked copy-on-write; text is shared. Static variables, globals, heap and stack all start identical, and diverge on first write. `exec()` throws that image away and builds a new one.

**Q14. What are ASLR and PIE?**

ASLR randomises the base of the stack, heap, mmap region and (for PIE) the executable, so an attacker cannot hard-code addresses. PIE builds the executable with position-independent code so it can be loaded anywhere. Cost: a little extra register pressure and indirection; benefit: a much harder exploit target.

### Linking and symbols

**Q15. How does the linker handle duplicate symbols?**

Two strong definitions (functions and initialised globals) give a *multiple definition* error. A strong definition beats weak ones. Multiple weak (or, with `-fcommon`, multiple uninitialised common) symbols: the linker picks one, and the largest size for common symbols.

**Q16. Why is `static int x;` in a header a problem?**

Each TU that includes it gets its own private copy, so "shared" state is silently duplicated and compilers warn about unused copies. Put `extern int x;` in the header and one definition in a `.c` file.

**Q17. How do dynamic libraries resolve calls (PLT and GOT)?**

Calls to external functions go through a PLT stub that jumps via a GOT entry. With lazy binding, the first call enters the dynamic linker, which resolves the symbol and patches the GOT; later calls jump directly. `-Wl,-z,now` binds everything at load time, and full RELRO then makes the GOT read-only, a common hardening choice for network products.

**Q18. What is symbol interposition, and how do you prevent it?**

A symbol with default visibility in a shared object may be overridden by another object that appears earlier in the lookup order (such as an `LD_PRELOAD` library). It also forces calls within the library to go through the PLT. Use `-fvisibility=hidden`, `-Bsymbolic` or `static` to avoid it.

**Q19. `static inline` vs `inline` vs macro?**

`static inline` gives each TU its own internal definition, which is safe in headers. Plain C99 `inline` needs one `extern inline` definition in a single TU. A macro has no type checking and can evaluate arguments twice. Inline is a hint; the compiler decides.

**Q20. Is a `static` local variable thread-safe?**

No. Reads and writes are ordinary memory accesses, so concurrent callers race. Use `_Thread_local`, pass a context pointer, or protect with a lock or atomics. This is why POSIX has `_r` variants such as `strtok_r` and `localtime_r`, and why `inet_ntop` (caller supplies the buffer) replaces `inet_ntoa` (returns a static buffer).

### More frequently asked

**Q21. What is a stack frame and what is in it?**

Each function call gets a *frame* on the stack. It holds the return address, optionally the saved frame pointer (`rbp`), saved callee-saved registers, local variables, spilled arguments and padding to keep the stack 16-byte aligned at calls. On x86-64 System V the first six integer or pointer arguments are passed in `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9` (floating-point in `xmm0` to `xmm7`) and the result comes back in `rax`, so many "locals" never touch the stack at all with optimisation on. Deep recursion or large locals exhaust the stack.

**Q22. Is it safe to return a string literal from a function?**

Yes. A string literal has static storage duration, so `const char *name(void) { return "eth0"; }` is fine. Compare puzzle P6, where the array is an automatic local. Return it as `const char *`, because modifying the literal is undefined behaviour.

**Q23. Does `static` on a function change anything besides visibility?**

Yes. Because no other file can call it, the compiler knows every call site: it can inline it, delete it if unused (and warn), and use cheaper calling conventions. It also avoids symbol clashes and keeps the dynamic symbol table small.

**Q24. `const`, `#define` or `enum` for constants?**

`#define` is textual, has no type and no scope. `enum` gives typed-ish integer constants with scope, usable in array sizes and `case` labels. A `const int` has a type and scope, but in C it is *not* a constant expression, so it cannot size a file-scope array or be a `case` label (C++ allows both). A `const` global in a header also has external linkage in C, so define it in one `.c` file, or use `static const` or `enum` in the header.

**Q25. What is a memory leak vs a dangling pointer vs a double free?**

A leak is allocated memory that is no longer reachable and never freed. A dangling pointer refers to memory whose lifetime has ended (freed heap block or returned local). A double free calls `free` twice on the same block and corrupts the allocator. Tools: Valgrind, AddressSanitizer (`-fsanitize=address`) and LeakSanitizer.

## 7. Code puzzles with explanations

Cover the answer, predict the output, then check. Assumes x86-64 Linux, LP64, GCC or Clang.

**P1. What does this print?**

```c
void f(void) {
    static int n = 0;
    int m = 0;
    n++; m++;
    printf("%d %d\n", n, m);
}
int main(void) { f(); f(); f(); }
```

Output is `1 1`, `2 1`, `3 1`. `n` is initialised once and persists; `m` is recreated and reset on every call.

**P2. Reaching a shadowed global.**

```c
int x = 10;
int main(void) {
    int x = 20;
    {
        extern int x;
        printf("%d\n", x);
    }
    printf("%d\n", x);
}
```

Output is `10` then `20`. The inner `extern int x;` has a prior visible declaration that has no linkage (the local `x`), so the rule says the new declaration gets external linkage and refers to the global. This is the C way to reach a shadowed global, since there is no `::`.

**P3. sizeof on literals.**

```c
char a[] = "hello";
char *p  = "hello";
printf("%zu %zu\n", sizeof a, sizeof p);
```

Output is `6 8`. The array holds five letters plus the terminating NUL; the pointer is 8 bytes on LP64. The array's bytes are writable; the pointed-to literal is not.

**P4. Does this compile?**

```c
int a = 5;
int b = a;       // file scope
int main(void) { return b; }
```

No, in C. File-scope and static initialisers must be constant expressions, because the data is laid down in the image before `main` runs, and a variable (even a `const` one) is not a constant expression in C. The same code is legal in C++, which runs dynamic initialisation at start-up. Fix: `int b = 5;`, or assign inside `main`.

**P5. Static in recursion.**

```c
int f(int n) {
    static int depth = 0;
    depth++;
    if (n > 0) f(n - 1);
    return depth;
}
printf("%d\n", f(3));
```

Output is `4`. One shared `depth` is incremented by all four activations (n = 3, 2, 1, 0); the outermost call returns after the others have finished, so it reads 4.

**P6. What is wrong here?**

```c
char *ifname(void) {
    char buf[16] = "eth0";
    return buf;
}
```

`buf` is automatic, so it dies at return and the caller holds a dangling pointer (undefined behaviour; GCC warns). Fixes, in order of preference for protocol code: have the caller pass a buffer and size; or `malloc` (caller frees); or `static char buf[16]`, which works but is not reentrant or thread-safe.

**P7. Order the addresses.**

```c
int g_init = 1;
int g_bss;
int main(void) {
    int loc;
    int *h = malloc(4);
    printf("%p %p %p %p %p %p\n", (void*)main, (void*)"x",
           (void*)&g_init, (void*)&g_bss, (void*)h, (void*)&loc);
}
```

Typical ascending order: `main` (.text) < `"x"` (.rodata) < `&g_init` (.data) < `&g_bss` (.bss) < `h` (heap) < `&loc` (stack). With PIE and ASLR the whole executable and heap sit near `0x55...` and the stack near `0x7ff...`; the relative order holds, and the mmap region (libc) sits between heap and stack. Treat it as the usual layout, not a guarantee. (Casting a function pointer to `void *` is a common GCC extension; strict ISO C does not define it.)

**P8. A "shared" counter in a header.**

```c
// counter.h
static int count;
static inline void inc(void) { count++; }

// a.c:  inc(); inc();
// b.c:  printf("%d\n", count);   // prints?
```

It prints `0`. `static` gives each including TU its own private `count`, and `a.c` increments its own copy. Fix: `extern int count;` in the header and `int count;` in exactly one `.c`.

**P9. Same function name in two files.**

```c
// a.c: static void note(void){ puts("a"); }   void fa(void){ note(); }
// b.c: static void note(void){ puts("b"); }   void fb(void){ note(); }
```

Legal: internal linkage makes them two unrelated functions, and `fa` prints `a` while `fb` prints `b`. Remove `static` and the link fails with *multiple definition of `note`*. (Avoid names such as `log`, which clash with standard library functions.)

**P10. Initialised vs uninitialised size.**

```c
int big_a[1000000];          // (1)
int big_b[1000000] = {1};    // (2)
```

Both occupy 4 MB of memory at run time. (1) is `.bss` and adds nothing to the file; (2) is `.data`, so the executable grows by about 4 MB. That matters for firmware images on flash.

## 8. The protocol-developer angle (Senior / Staff follow-ups)

Senior and staff interviews at network vendors rarely stop at "what does static mean". They move to how these rules shape a packet path, a control plane or a multi-process NOS. Be ready to connect each concept to a design choice.

### Per-core and per-thread state

Globals in the fast path are shared mutable state: they need locks or atomics, and they cause cache-line bouncing. Prefer per-core or per-thread data, aggregated only when read.

```c
#include <stdalign.h>
#include <stdint.h>

// alignas on the first member makes the whole struct 64-byte aligned and padded to 64 bytes.
struct stats { alignas(64) uint64_t rx; uint64_t tx, drop; };   // sizeof == 64: one cache line
// GCC/Clang alternative: struct stats { uint64_t rx, tx, drop; } __attribute__((aligned(64)));

// One cache line per core: no false sharing. Lives in .bss.
static struct stats per_core[MAX_CORES];

static inline void count_rx(unsigned core) { per_core[core].rx++; }  // no lock

uint64_t total_rx(void) {            // slow path: aggregate
    uint64_t s = 0;
    for (unsigned i = 0; i < MAX_CORES; i++) s += per_core[i].rx;
    return s;
}
```

The alignment is part of the struct type, so every array element is padded to a full 64-byte line. Putting `alignas(64)` on the array instead would align only its first element, and the 24-byte elements would still share lines. (Writing `struct alignas(64) stats {...}` is only valid from C23, so the first-member form is the portable C11 spelling.) A reader summing the counters may see slightly stale values, which is acceptable for statistics. `_Thread_local` gives the same isolation per thread when threads are not pinned to cores.

### Memory in a long-running router process

- **No `malloc` on the packet path.** Allocation latency is unpredictable and arenas fragment over months of uptime. Use fixed-size pools or rings preallocated at start-up from `.bss` or hugepages.
- **Hugepages (2 MiB or 1 GiB)** cut TLB misses for large FIB, MAC and flow tables; DPDK maps its pools this way and pins them so they are never swapped.
- **NUMA**: memory is placed on the node of the thread that first touches it, so initialise a table from the core that will use it.
- **Large static tables** in `.bss` cost no file space but are zero pages until touched; a `static const` lookup table lives in `.rodata` and is shared by every process running that image.
- **Stacks are small.** A few 9 KB jumbo-frame buffers as locals (`char pkt[9216]`) in nested calls can exhaust a small 64 KiB or 256 KiB thread stack. Check with `-fstack-usage` and `-Wstack-usage=`, and avoid VLAs and `alloca` on untrusted lengths.

### Shared memory between processes

NOS designs split protocols into separate processes sharing state through shared memory. The mapping address differs in each process, so **raw pointers must never be stored in shared memory**; store offsets from the base, or use a fixed `MAP_FIXED_NOREPLACE` address by agreement.

```c
int fd = shm_open("/rib", O_CREAT | O_RDWR, 0600);
ftruncate(fd, SZ);
void *base = mmap(NULL, SZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

struct route { uint32_t pfx, len; uint32_t next_off; };  // offset, not a pointer
struct nh { uint32_t addr, ifindex; };

// r points at a struct route that lives inside the mapping
struct nh *nh = (struct nh *)((char *)base + r->next_off);   // rebuild the pointer locally
```

Also: put a version and a lock (robust process-shared mutex or a seqlock) in the header, and remember that a crashed writer can leave the data half-updated.

### volatile, signal handlers and ISRs

- `volatile` tells the compiler that every access is observable: right for memory-mapped device registers (`volatile uint32_t *reg = (void *)0xF0000000;`) and for a flag set from a signal handler (`volatile sig_atomic_t`).
- It is **not** a synchronisation primitive: it gives no atomicity and no ordering between threads. Use C11 `<stdatomic.h>` or memory barriers. To the question "is `volatile` enough for a shared counter?" the answer is no.
- A signal handler or ISR may only call async-signal-safe functions. No `malloc`, `printf` or locks: the interrupted code may already be inside `malloc` holding its internal lock, so the handler would deadlock or corrupt state.
- `static` data written by a handler and read by the main loop needs `sig_atomic_t` or an atomic type.

### Self-registration tables with linker sections

A common NOS idiom: each protocol module registers itself without a central list, by placing a pointer in a named section. GNU ld defines `__start_<name>` and `__stop_<name>` for any section whose name is a valid C identifier.

```c
struct proto_ops { const char *name; int (*rx)(void *pkt); };

#define REGISTER_PROTO(ops)                                          \
    static const struct proto_ops *const reg_##ops                   \
        __attribute__((used, section("proto_tbl"))) = &(ops)

extern const struct proto_ops *const __start_proto_tbl[], *const __stop_proto_tbl[];

static const struct proto_ops ospf_ops = { "ospf", ospf_rx };
REGISTER_PROTO(ospf_ops);          // static: internal linkage, yet reachable via the section

void dispatch_all(void *pkt) {
    for (const struct proto_ops *const *p = __start_proto_tbl; p < __stop_proto_tbl; p++)
        (*p)->rx(pkt);
}
```

This ties together linkage (the registration object is `static`), storage (`const` data in a custom read-only section) and the linker. Variants use `__attribute__((constructor))`, which runs at load time before `main`, though ordering across files is unspecified.

### Plugins and shared objects

With `dlopen`, `RTLD_LOCAL` keeps a plugin's symbols private, while `RTLD_GLOBAL` makes them available to later loads. Combine this with `-fvisibility=hidden` and an explicit export macro so a plugin cannot accidentally interpose a symbol used by the host, and so two plugins that define `static` helpers of the same name remain independent.

### Alignment and packet headers

Casting a `char *` into a packed header (`(struct ip *)buf`) can produce an unaligned access, which is harmless on x86 but traps or is slow on some ARM and MIPS targets. Use `memcpy` into a local struct (the compiler folds it), or `__attribute__((packed))` with care, and convert with `ntohs`/`ntohl`.

### Likely design questions

| Question | What a strong answer covers |
| --- | --- |
| Per-flow counters at 100 Mpps with low contention? | per-core arrays padded to cache lines, aggregation on the slow path, no locks or atomics on the hot path, hugepage-backed flow table, first-touch NUMA placement |
| Read-mostly FIB updated by the control plane? | RCU or double-buffering with an atomic pointer swap, readers never lock, deferred free after a grace period |
| A daemon's RSS grows over weeks. How do you debug? | `pmap` and `/proc/<pid>/smaps` to see which region (heap vs mmap vs stack), `malloc_stats`, heap profilers, ASan/LSan in test, allocator arenas and fragmentation, then fix with pools |
| Corrupted data in a global, intermittently? | data race (TSan), buffer overflow from a neighbouring `.data`/`.bss` object (check `nm -n`), signal handler touching non-atomic state |
| Two modules define the same symbol name | `multiple definition` or silent interposition; fix with `static`, namespaced prefixes, hidden visibility |
| Why did the build pass but the link fail with `undefined reference`? | declaration without definition, wrong link order for static libraries, missing `-l`, C++ name mangling without `extern "C"` |

### Last-minute checklist

- Say the three properties separately every time: **scope** (name, compile time), **lifetime** (object, run time), **linkage** (symbol, link time).
- `static` = hide at file scope, persist in a block. `extern` = declare, never allocate.
- `.text` r-x, `.rodata` r--, `.data` and `.bss` rw-; `.bss` costs no file space; the heap grows up, the stack grows down.
- Literals and file-scope `const` are in `.rodata`; `char *p = "x"` is read-only, `char a[] = "x"` is a writable copy.
- Locals are uninitialised; statics and globals are zero.
- Headers declare; one `.c` defines; `static inline` for header functions; `-fvisibility=hidden` for libraries.
- A static local is neither reentrant nor thread-safe: use `_Thread_local`, a context struct, or caller-provided buffers.
- Be ready to draw the memory map and to run `nm`, `size`, `readelf` and `/proc/<pid>/maps` from memory.
