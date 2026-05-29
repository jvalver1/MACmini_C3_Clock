---
name: ESP32 FreeRTOS Development Guide
description: Best practices and API specifications for writing high-performance multi-threaded applications using Espressif's custom FreeRTOS on the ESP32 and ESP32-C3.
---

# ESP32 FreeRTOS Development Guide

This guide details the core principles, practices, and configuration options for developing multi-threaded firmware using Espressif's custom FreeRTOS implementation.

---

## 1. Espressif FreeRTOS vs. Vanilla FreeRTOS

Espressif's FreeRTOS is a customized fork designed to handle dual-core Symmetric Multiprocessing (SMP) on standard ESP32/ESP32-S3 chips, while retaining single-core operation on chips like the ESP32-C3.

### **Key Architectural Differences**
1. **Multicore Scheduling**: Vanilla FreeRTOS is strictly single-core. Espressif FreeRTOS runs a single scheduler across both CPU cores (Core 0 / PRO_CPU and Core 1 / APP_CPU) to schedule tasks dynamically or pin them specifically.
2. **ESP32-C3 Specifics**: The ESP32-C3 uses a single-core RISC-V processor. While the same FreeRTOS API is used, there is only Core 0. Any task pinning parameters targeting Core 1 are ignored, and all tasks share a single CPU core.
3. **Stack Sizes in Bytes**: In vanilla FreeRTOS, the stack size passed to task creation functions is defined in **words** (4 bytes on 32-bit platforms). In ESP-IDF FreeRTOS, stack sizes are defined in **bytes**. This is a common source of memory exhaustion.
4. **Spinlocks for Critical Sections**: On dual-core ESP32 chips, disabling interrupts on one core is insufficient to prevent concurrent access from the other core. Critical sections use spinlocks (`portMUX_TYPE`) to protect shared memory.

---

## 2. Task Management & Scheduling

Tasks are the fundamental execution block. In FreeRTOS, they are priority-preemptive threads.

### **Task Creation APIs**

Always prefer `xTaskCreatePinnedToCore()` over `xTaskCreate()` when working on dual-core chips to prevent tasks from shifting between cores, which can cause cache invalidations.

```cpp
BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t pvTaskCode,      // Function pointer to the task
    const char * const pcName,     // Descriptive name (for debugging)
    const uint32_t usStackDepth,    // Stack depth in BYTES
    void * const pvParameters,      // Parameter passed to the task
    UBaseType_t uxPriority,         // Task priority (higher number = higher priority)
    TaskHandle_t * const pxCreatedTask, // Task handle
    const BaseType_t xCoreID        // Core ID: 0, 1, or tskNO_AFFINITY
);
```

#### **Core Allocation Rules**
* **Core 0 (PRO_CPU)**: Handles background system tasks like Wi-Fi, Bluetooth, TCP/IP stack, and system interrupts. Avoid running heavy, non-yielding user computations here.
* **Core 1 (APP_CPU)**: Dedicated to user application tasks, UI updates, and sensor polling.
* **tskNO_AFFINITY**: The scheduler will run the task on whichever CPU core is free. Recommended only for stateless helper tasks.
* **ESP32-C3 (Single Core)**: Set `xCoreID` to `0` or `tskNO_AFFINITY`. 

---

## 3. Yielding & CPU Watchdogs (TWDT)

Because FreeRTOS is a priority-preemptive operating system, a high-priority task will run indefinitely unless it actively yields.

### **The Task Watchdog Timer (TWDT)**
The TWDT monitors execution. If a CPU core's Idle task is starved of CPU time (e.g., because a high-priority task is stuck in a `while(true)` loop without blocking), the TWDT triggers a system panic:
`E (xxxx) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:`

### **How to Properly Yield the CPU**
1. **Never use active busy-waiting (`while(cond);` or `delayMicroseconds()`) for long intervals**.
2. **Use `vTaskDelay()`**: Moves the task into the "Blocked" state, allowing lower-priority tasks (including the Idle task) to run.
   ```cpp
   // Delay for 100 milliseconds
   vTaskDelay(pdMS_TO_TICKS(100));
   ```
3. **Use Event Groups or Semaphores**: Block the task until an interrupt or another task releases it.

---

## 4. Stack Size and Priority Allocation

### **Stack Size Guidelines**
Stack space holds local variables, function calls, and interrupt contexts.
* **Minimum Stack**: `2048 bytes` (for simple tasks).
* **Medium Tasks (JSON parsing, encryption, small buffers)**: `4096 to 8192 bytes`.
* **Large Tasks (WiFi/HTTP requests, Graphics rendering)**: `8192 to 16384 bytes`.
* **Stack Overflow Prevention**: If a task exceeds its stack, the system triggers a Stack Overflow panic. Keep large arrays or structs on the heap (via `malloc`) instead of local stack variables.

### **Priority Guidelines**
In ESP-IDF, priority values range from `0` (lowest, Idle task) to `configMAX_PRIORITIES - 1` (usually 25).
* **Low Priority (1 - 4)**: Background logging, housekeeping, telemetry.
* **Normal Priority (5 - 9)**: Main user interface, buttons, system logic.
* **High Priority (10 - 15)**: Time-critical operations, protocol parsing, low-latency device control.
* **Real-time / Driver Priority (16+)**: reserved for time-critical ISR-driven queues.

---

## 5. Inter-Task Synchronization & Locking

When sharing data between tasks (especially cross-core tasks), standard C variables are not thread-safe.

### **Mutexes vs. Semaphores vs. Notifications**

| Primitive | Use Case | Performance Cost | Core-Safe? |
| :--- | :--- | :--- | :--- |
| **Mutex (`SemaphoreHandle_t`)** | Mutual exclusion (protecting shared variables/resources). Supports priority inheritance. | Medium | Yes |
| **Binary Semaphore** | Signaling between ISR/Task or Task/Task. | Medium | Yes |
| **Task Notifications** | Light-weight signaling directly to a target task. | **Low** (Fastest, avoids heap allocation) | Yes |

#### **Example: Using a Mutex**
```cpp
SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();

void task1(void *pvParameters) {
    while(true) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            // Access shared resource safely
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

#### **Example: Task Notifications (Recommended for Speed)**
Instead of creating a binary semaphore, signal a task directly by its handle:
```cpp
TaskHandle_t xTaskToNotify = NULL;

// In Receiver Task:
void rxTask(void *pv) {
    uint32_t ulNotificationValue;
    while(true) {
        // Wait blockingly for notification
        if (xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, portMAX_DELAY) == pdTRUE) {
            // Signal received!
        }
    }
}

// In Sender Task / ISR:
void txTask() {
    xTaskNotifyGive(xTaskToNotify); // Fast and light
}
```

---

## 6. Interrupts (ISRs) and FreeRTOS

Interrupt Service Routines (ISRs) run in a hardware context, interrupting the current task.

### **Critical Rules for ISRs**
1. **Never block inside an ISR**. You cannot call `vTaskDelay()`, `xSemaphoreTake(..., portMAX_DELAY)`, or any blocking function.
2. **Only call FreeRTOS APIs that end with `FromISR`** (e.g., `xQueueSendFromISR`, `xSemaphoreGiveFromISR`, `vTaskNotifyGiveFromISR`).
3. **Keep ISRs extremely short**. Perform time-consuming work inside a deferred task triggered by a Task Notification or Semaphore.
4. **Use the `pxHigherPriorityTaskWoken` flag** to yield immediately upon ISR completion if a higher priority task was unblocked:
   ```cpp
   void IRAM_ATTR gpio_isr_handler(void* arg) {
       BaseType_t xHigherPriorityTaskWoken = pdFALSE;
       vTaskNotifyGiveFromISR(xTaskToNotify, &xHigherPriorityTaskWoken);
       if (xHigherPriorityTaskWoken) {
           portYIELD_FROM_ISR(); // Yield to the notified task instantly
       }
   }
   ```
5. **IRAM_ATTR Attribute**: Place ISR functions in Instruction RAM (IRAM) using the `IRAM_ATTR` macro so they can execute even during Flash write/erase operations.
