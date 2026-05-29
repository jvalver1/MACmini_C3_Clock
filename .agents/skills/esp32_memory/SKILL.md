---
name: ESP32 Memory Management & Diagnostics Guide
description: Architectural overview, capabilities-based memory allocation, memory leak detection using heap tracing, heap corruption diagnostics, and common symptoms in ESP-IDF.
---

# ESP32 Memory Management & Diagnostics Guide

This guide details the memory architecture of the ESP32 series microcontrollers and provides a framework for capabilities-based memory allocation, leak detection, and heap corruption diagnostics.

---

## 1. ESP32 Memory Architecture

The ESP32 features a heterogeneous memory layout with a unified address space. The internal SRAM is split into three blocks (SRAM1, SRAM2, SRAM3) which are mapped as:
- **DRAM (Data RAM)**: Read/write memory used for task stacks, heap, global variables, and DMA buffers.
- **IRAM (Instruction RAM)**: Execution-only memory used for critical code, interrupt service routines (ISRs), and code loaded from Flash.
- **D/IRAM**: Shared address space mapped to both buses.
- **External PSRAM (SPI RAM)**: Optional external RAM accessible over the SPI bus (slower access than internal SRAM, not DMA-capable).

---

## 2. Capabilities-Based Allocation (`heap_caps_malloc`)

Because different regions of RAM support different hardware buses and alignment rules, ESP-IDF uses a capability-based allocator. Always use `heap_caps_malloc(size, capabilities)` to request memory with specific hardware attributes.

### **Common Capability Flags**

| Flag | Meaning | Use Case |
| :--- | :--- | :--- |
| **`MALLOC_CAP_8BIT`** | Byte-addressable memory (Standard Data RAM). | Default fallback (equivalent to standard `malloc`). |
| **`MALLOC_CAP_DMA`** | Memory suitable for DMA hardware (SPI, I2S, etc.). | SPI display framebuffers, DAC/ADC buffers. Excludes PSRAM. |
| **`MALLOC_CAP_32BIT`** | Memory accessible strictly in 32-bit aligned chunks. | Large integer arrays, pointer buffers. Mapped to IRAM. |
| **`MALLOC_CAP_SPIRAM`** | Specifically requests external SPI RAM. | Large image buffers, caching, databases. |
| **`MALLOC_CAP_INTERNAL`** | Force allocation in internal SRAM instead of PSRAM. | Performance-critical variables or DMA buffers. |

### **Best Practices & IRAM Data Warning**
To maximize free DRAM, you can allocate 32-bit aligned arrays (such as pointer lists or `int32_t` arrays) into instruction RAM (IRAM) using `MALLOC_CAP_32BIT`.
> [!CAUTION]
> 1. **LoadStoreError**: Any unaligned read/write (e.g., trying to access a `char` byte inside a 32-bit block) will immediately trigger a hardware exception (`LoadStoreError`).
> 2. **No Floats**: Do not store float variables in `MALLOC_CAP_32BIT` memory; ESP32 floating-point assembly instructions are not capable of accessing IRAM.

---

## 3. Heap Corruption and Poisoning

Heap corruption occurs when code writes past the boundary of an allocated buffer. This is notoriously difficult to diagnose because the crash often occurs much later, during a future allocation or free.

### **Corruption Detection Levels (`menuconfig`)**
In `menuconfig` under `Component config -> Heap Memory Debugging -> CONFIG_HEAP_CORRUPTION_DETECTION`, you can configure the debug level:

1. **Basic (Default)**: Checks structural metadata. Triggers an assertion on double-frees or corrupt block headers.
2. **Light Impact (Canaries)**: Places hidden "canary" bytes immediately before and after every allocated block. Validates them on `free()`.
3. **Comprehensive**: Overwrites memory blocks with patterns upon allocation and deallocation to detect use-after-free and uninitialized access.

### **Magic Debug Patterns & Canary Values**

If you enable Light or Comprehensive debugging, monitor serial console logs for these exact magic patterns:

*   **`0xABBA1234` (Head Canary)**: The 4-byte signature placed directly before your allocated buffer. If changed, an underrun (writing before the start of the buffer) occurred.
*   **`0xBAAD5678` (Tail Canary)**: The 4-byte signature placed directly after your allocated buffer. If changed, an overrun (writing past the end of the buffer) occurred.
*   **`0xCECECECE` (Uninitialized Memory)**: In Comprehensive mode, freshly allocated memory is filled with `0xCE`. If a crash dump references pointer addresses containing `0xCECECECE`, you are reading uninitialized variables.
*   **`0xFEFEFEFE` (Use-After-Free)**: In Comprehensive mode, freed memory is filled with `0xFE`. If your code crashes trying to read or dereference `0xFEFEFEFE`, your task is illegally referencing a pointer that has already been passed to `free()`.

---

## 4. Memory Leak Diagnostics & Heap Tracing

A memory leak occurs when `malloc` is called without a matching `free`, slowly consuming all available RAM.

### **The Heap Tracing API**
ESP-IDF includes a built-in tracer that logs all allocations and deallocations.

#### **Implementation Steps**
1. Enable `CONFIG_HEAP_TRACING_STANDALONE` in `menuconfig`.
2. Initialize the tracing subsystem and allocate a trace buffer in your code:
   ```cpp
   #include "esp_heap_trace.h"
   #define NUM_RECORDS 100
   static heap_trace_record_t trace_record[NUM_RECORDS];
   
   // In initialization:
   heap_trace_init_standalone(trace_record, NUM_RECORDS);
   ```
3. Surround the code segment you want to analyze:
   ```cpp
   // Start recording only allocations that haven't been freed
   heap_trace_start(HEAP_TRACE_LEAKS);
   
   // ... Run suspected leaky logic ...
   
   heap_trace_stop();
   heap_trace_dump(); // Prints all callers/callstacks of leaked allocations
   ```

### **Common False Positives**
When reviewing `heap_trace_dump()` logs, ignore the following allocations:
- **First-use allocations**: The first call to functions like `printf()` or any `stdio` command will allocate a permanent RTOS mutex lock. This is normal and is not a leak.
- **Networking buffers**: Wi-Fi, Bluetooth, or LWIP stacks allocate packet buffers dynamically. If a packet is in transit when tracing stops, it will register as a "leak" but will be freed automatically shortly after.
- **TCP connections** in the `TIME_WAIT` state.

---

## 5. Advanced Troubleshooting Techniques

### **1. Manual Integrity Scanning**
If a memory crash occurs randomly, insert manual integrity sweeps at critical lifecycle points in your tasks:
```cpp
// Triggers an immediate assertion crash if any canary is corrupted in the heap
heap_caps_check_integrity_all(true);
```
Sprinkling this API call throughout your main loops will help isolate exactly which function is responsible for corrupting the buffer.

### **2. Hardware Watchpoints (Halt on Write)**
If a specific variable is being overwritten, set a hardware data watchpoint.
```cpp
// Halts the CPU instantly when any write instruction targets the variable address
esp_set_watchpoint(0, (void *)&corrupted_variable, 4, ESP_WATCHPOINT_STORE);
```
Once triggered, the JTAG or GDB debugger will point to the exact line of assembly code that executed the illegal overwrite.

### **3. Allocation Failure Callbacks**
Use `heap_caps_register_failed_alloc_callback()` to register a custom failure hook. When an allocation fails, write system registers or the current free heap metrics to flash/serial before performing a soft restart:
```cpp
void my_failed_alloc_callback(size_t size, uint32_t caps, const char *function_name) {
    ESP_LOGE("MEM", "Failed to allocate %d bytes (caps: 0x%X) in %s", size, caps, function_name);
    ESP_LOGI("MEM", "Free Heap: %d, Min Free: %d", 
             esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
}
```

### **4. Assessing Heap Fragmentation**
If `malloc` fails despite `esp_get_free_heap_size()` indicating plenty of free space, your heap is fragmented.
- Compare **Total Free Space** against **Largest Free Block**:
  ```cpp
  size_t free_total = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  size_t free_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
  ```
- If `free_block` is significantly smaller than `free_total`, the memory is fragmented into many small disjointed blocks. Avoid frequent small allocations and deallocations; allocate memory statically, or use object pooling for frequent buffers.
