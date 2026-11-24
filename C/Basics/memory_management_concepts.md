# Stack Overflow vs Stack Corruption vs Heap Overflow

A quick reference for telling apart three commonly confused memory errors in C/C++ (and other low-level) programs.

## Table of Contents

- [Terminology Note](#terminology-note)
- [1. Stack Overflow vs Stack Corruption](#1-stack-overflow-vs-stack-corruption)
  - [Stack Overflow](#stack-overflow)
  - [Stack Corruption](#stack-corruption)
  - [Stack Frame Layout](#stack-frame-layout)
- [2. Stack Overflow vs Heap Overflow](#2-stack-overflow-vs-heap-overflow)
  - [Stack Overflow (Recap)](#stack-overflow-recap)
  - [Heap Overflow](#heap-overflow)
- [3. Quick Comparison](#3-quick-comparison)
- [4. Related Terms](#4-related-terms)

---

## Terminology Note

> [!NOTE]
> The term **"stack overflow"** is used in two different ways in the wild:
>
> - **Stack exhaustion** (used in this document): the stack runs out of space because it grew beyond its limit.
> - **Stack buffer overflow**: an out-of-bounds write to a buffer on the stack. In this document this is called **stack corruption**.
>
> Many security articles (and CVE descriptions) use "stack overflow" to mean the second case. Always check which meaning is intended.

---

## 1. Stack Overflow vs Stack Corruption

### Stack Overflow

- Stack overflow occurs when a program exceeds the allocated stack size, usually due to excessive stack memory usage.
- Stack overflow is **resource exhaustion** of the stack.

**Common Causes**

- Infinite or very deep recursion
- Very large local variables (arrays/structs)
- Too many nested function calls
- Small thread stack size (common in embedded/network OS)
- Large `alloca()` calls or variable-length arrays (VLAs) with unchecked sizes

**What Happens Internally**

- Stack grows until it hits:
  - Stack limit (set by OS, e.g. 8 MB by default for the main thread on many Linux systems; check with `ulimit -s`)
  - Guard page
- OS raises `SIGSEGV` / `SIGBUS` (on Windows: `EXCEPTION_STACK_OVERFLOW`)
- Program crashes immediately

**Example**

```c
void func() {
    char buf[1024 * 1024 * 1024];   // 1 GB local array, far larger than the stack
    foo();                          // any function call touches the stack and faults
}

int main() {
    func();
}
```

**Example: runaway recursion**

```c
int recurse(int n) {
    char pad[256];
    pad[0] = (char)n;
    return recurse(n + 1) + pad[0];   // no base case: grows until the stack limit
}
```

**How to Detect**

- Crash backtrace in `gdb` showing thousands of repeated frames
- Compiler warnings: `-Wstack-usage=<bytes>`, `-Wframe-larger-than=<bytes>`, `-fstack-usage`
- AddressSanitizer (`-fsanitize=address`) reports `stack-overflow`
- Embedded/RTOS: stack high-water-mark APIs (e.g. FreeRTOS `uxTaskGetStackHighWaterMark`), `configCHECK_FOR_STACK_OVERFLOW`, or filling the stack with a known pattern and checking how much is untouched

**How to Prevent**

- Replace deep recursion with iteration or enforce a recursion depth limit
- Move large buffers to the heap or to static storage
- Increase the thread stack size (e.g. `pthread_attr_setstacksize`, RTOS task stack parameter)
- Build with `-fstack-clash-protection` to stop large frames from jumping over the guard page

### Stack Corruption

- Stack corruption occurs when a program writes outside its intended stack memory, overwriting:
  - Return address
  - Saved registers
  - Frame pointer
  - Other stack variables
- The stack still exists, but its contents are damaged.
- Stack corruption is a **memory safety violation**, not size exhaustion.

**Common Causes**

- Buffer overflow on stack arrays
- Incorrect pointer arithmetic
- `memcpy` / `strcpy` without bounds check
- Writing past local variable limits
- Off-by-one errors (e.g. `for (i = 0; i <= N; i++)` on an `N`-element array)
- Unsafe format strings or `gets()`

**What Happens Internally**

- Overwrites:
  - Adjacent local variables
  - Saved frame pointer
  - Return address
- Program may:
  - Crash later
  - Jump to a random address
  - Behave unpredictably (this is undefined behavior)

**Example**

```c
void func() {
    char buf[8];
    strcpy(buf, "this_is_a_very_long_string");   // 27 bytes (including '\0') into an 8-byte buffer
}
```

**Safer Version**

```c
void func() {
    char buf[8];
    snprintf(buf, sizeof(buf), "%s", "this_is_a_very_long_string");   // truncates safely
}
```

**How to Detect**

- Runtime message such as `*** stack smashing detected ***: terminated` (stack canary triggered, usually ends in `SIGABRT`)
- AddressSanitizer reports `stack-buffer-overflow`
- Compiler warnings such as `-Wstringop-overflow`, `-Warray-bounds`
- Core dumps with a garbage return address or corrupted backtrace

**How to Prevent / Mitigate**

- Use bounded APIs (`snprintf`, `strlcpy`, `memcpy` with verified sizes) and always check lengths
- Compile with `-fstack-protector-strong` (stack canaries) and `-D_FORTIFY_SOURCE=2` (with optimization)
- Keep OS/platform mitigations enabled: NX (non-executable stack), ASLR, PIE, shadow stacks / CET where available
- These mitigations make exploitation harder, but they do not fix the underlying bug

> **"Overflow exhausts the stack; corruption damages the stack."**

### Stack Frame Layout

A simplified view of a typical x86-64 frame (the stack grows toward lower addresses, but buffer writes move toward higher addresses):

```text
  High addresses
  +-----------------------------+
  | Caller's frame              |
  +-----------------------------+
  | Return address              |  <-- overwritten by a large overflow
  +-----------------------------+
  | Saved frame pointer         |
  +-----------------------------+
  | Stack canary (if enabled)   |  <-- checked before the function returns
  +-----------------------------+
  | char buf[8]                 |  <-- writes beyond this move upward
  +-----------------------------+
  Low addresses (stack grows this way)
```

---

## 2. Stack Overflow vs Heap Overflow

### Stack Overflow (Recap)

- Stack overflow occurs when a program exceeds the allocated stack size, usually due to excessive stack memory usage.
- Stack overflow is **resource exhaustion** of the stack.

**Common Causes**

- Infinite or very deep recursion
- Very large local variables (arrays/structs)
- Too many nested function calls
- Small thread stack size (common in embedded/network OS)

**What Happens Internally**

- Stack grows until it hits:
  - Stack limit (set by OS)
  - Guard page
- OS raises `SIGSEGV` / `SIGBUS`
- Program crashes immediately

**Characteristics**

- Deterministic crash
- Happens immediately
- Easy to reproduce
- Rarely exploitable (stack-clash attacks are a notable exception)

**Example**

```c
void func() {
    char buf[1024 * 1024 * 1024];
    foo();
}

int main() {
    func();
}
```

See [Stack Overflow](#stack-overflow) above for detection and prevention.

### Heap Overflow

- Heap overflow occurs when a program writes beyond the bounds of a heap-allocated buffer.

**Common Causes**

- Incorrect `malloc()` size
- Unsafe `memcpy`, `strcpy`
- Wrong packet length validation
- Integer overflow in size calculations (e.g. `malloc(count * sizeof(item))` wrapping around to a small value)
- Off-by-one errors, such as forgetting the terminating `'\0'`

**What Happens Internally**

- Overwrites adjacent heap objects
- May corrupt allocator metadata (chunk sizes, free-list pointers)
- Crash may happen later (often inside a later `malloc` / `free`, far from the faulty write)

**Characteristics**

- Non-deterministic crash
- Hard to reproduce
- Often exploitable
- Dangerous security bug

**Example**

```c
char *buf = malloc(16);
memcpy(buf, data, 64);   // heap overflow: writes 48 bytes past the end of the allocation
```

**Safer Version**

```c
size_t len = 64;
char *buf = malloc(len);
if (buf == NULL) {
    /* handle allocation failure */
}
memcpy(buf, data, len);   // size matches the allocation
```

**Heap Layout Illustration**

```text
  +-------------+--------------------+-------------+--------------------+
  | chunk header| buf (16 bytes)     | chunk header| next object        |
  +-------------+--------------------+-------------+--------------------+
                 ^ writing 64 bytes here spills into the next header and object
```

**How to Detect**

- AddressSanitizer reports `heap-buffer-overflow`
- Valgrind (`memcheck`)
- Allocator integrity errors, e.g. glibc: `malloc(): corrupted top size`, `free(): invalid next size`, `corrupted size vs. prev_size` (usually followed by `SIGABRT`)
- Debug allocators and tools such as `MALLOC_CHECK_`, `MALLOC_PERTURB_`, Electric Fence, GWP-ASan

**How to Prevent**

- Validate all external lengths (packet sizes, file headers) before allocating or copying
- Check for multiplication overflow before allocating (`calloc`, `reallocarray`, or `__builtin_mul_overflow`)
- Use bounded copy functions and track buffer capacity alongside the pointer
- Fuzz test and run under sanitizers in CI
- Consider memory-safe languages or safer abstractions (e.g. `std::vector`, `std::string`) for new code

> **"Stack overflow exhausts memory; heap overflow corrupts memory."**

> [!NOTE]
> Heap overflow is not the same as **heap exhaustion** (running out of memory so `malloc` returns `NULL`). Exhaustion is a resource problem; heap overflow is an out-of-bounds write.

---

## 3. Quick Comparison

| Aspect | Stack Overflow | Stack Corruption | Heap Overflow |
|---|---|---|---|
| Nature | Resource exhaustion | Memory safety violation | Memory safety violation |
| Memory region | Stack | Stack | Heap |
| Root cause | Too much stack use (deep recursion, huge locals) | Out-of-bounds write to a stack buffer | Out-of-bounds write to a heap buffer |
| Typical symptom | `SIGSEGV` / `SIGBUS` at the stack limit | Crash on return, canary abort (`SIGABRT`), or erratic behavior | Crash or abort inside a later `malloc` / `free`, or silent data corruption |
| Crash timing | Immediate | Often later (at function return) | Often later |
| Reproducibility | Easy (deterministic) | Varies | Hard (non-deterministic) |
| Exploitability | Rarely | Often | Often |
| Typical fix | Limit recursion, shrink or move buffers, enlarge stack | Bounds checks, safe APIs, stack protector | Validate sizes, bounds checks, safe allocation patterns |

---

## 4. Related Terms

| Term | Meaning |
|---|---|
| Stack buffer overflow | Out-of-bounds write to a stack array (this document calls it stack corruption) |
| Stack clash | Attack where the stack is made to collide with another memory region by jumping over the guard page |
| Heap exhaustion / out of memory | Heap has no space left, `malloc` returns `NULL` |
| Use-after-free | Accessing heap memory after it has been freed |
| Double free | Freeing the same heap block twice, corrupting allocator state |
| Integer overflow | Arithmetic wraparound that leads to undersized allocations or bad bounds checks |
| Stack canary | Known value placed before the return address and checked before returning, to detect stack corruption |
