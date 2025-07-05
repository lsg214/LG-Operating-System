/*
 * kernel.h - Main kernel header file
 * Contains function declarations and constants for the kernel
 */

#ifndef KERNEL_H
#define KERNEL_H

// Define our own types since we can't use standard library
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;
typedef uint32_t size_t;
typedef uint32_t uintptr_t;

#define NULL ((void*)0)

// VGA Constants
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// VGA Colors
typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
} vga_color;

// Memory Management
#define HEAP_START 0x200000  // 2MB
#define HEAP_END   0x400000  // 4MB
#define HEAP_SIZE  (HEAP_END - HEAP_START)

// Function Declarations

// String utilities
size_t strlen(const char* str);
void* memset(void* bufptr, int value, size_t size);
int strcmp(const char* str1, const char* str2);
char* strcpy(char* dest, const char* src);

// Terminal functions
void terminal_initialize(void);
void terminal_setcolor(uint8_t color);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_clear(void);
void terminal_scroll(void);

// Output functions
void printf(const char* format, ...);
void putchar(char c);
void puts(const char* str);

// Memory management
void* kmalloc(size_t size);
void kfree(void* ptr);
void* kmalloc_aligned(size_t size, size_t alignment);
void print_memory_info(void);
size_t get_available_memory(void);

// System functions
void kernel_panic(const char* message);
void kernel_halt(void);
void kernel_reboot(void);

// VGA helper functions
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

// Utility macros
#define UNUSED(x) ((void)(x))
#define PANIC(msg) kernel_panic(msg)
#define ASSERT(condition) do { \
    if (!(condition)) { \
        kernel_panic("Assertion failed: " #condition); \
    } \
} while(0)

// Kernel version information
#define KERNEL_NAME "MyOS"
#define KERNEL_VERSION "0.1.0"
#define KERNEL_AUTHOR "OS Developer"

#endif // KERNEL_H