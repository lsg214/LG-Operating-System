/*
 * kernel.c - Minimal C Kernel
 * A simple kernel that provides basic screen output and memory management
 */

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

// VGA text mode constants
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// VGA color constants - Fixed: use consistent enum declaration
enum vga_color {
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
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

// Terminal state
static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

// Simple memory allocator state
static uint8_t* heap_start = (uint8_t*)0x200000;  // Start heap at 2MB
static uint8_t* heap_end = (uint8_t*)0x400000;    // End heap at 4MB
static uint8_t* heap_current = (uint8_t*)0x200000;

// Function declarations - Fixed: added missing declarations
void init_interrupts(void);
void init_shell(void);

// VGA helper functions
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

// String utility functions
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

void* memset(void* bufptr, int value, size_t size) {
    unsigned char* buf = (unsigned char*) bufptr;
    for (size_t i = 0; i < size; i++)
        buf[i] = (unsigned char) value;
    return bufptr;
}

// Terminal initialization and control
void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t*) VGA_MEMORY;
    
    // Clear the screen
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_scroll(void) {
    // Move all lines up by one
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = terminal_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    // Clear the last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
    } else {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            if (++terminal_row == VGA_HEIGHT) {
                terminal_scroll();
                terminal_row = VGA_HEIGHT - 1;
            }
        }
    }
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

// Simple printf implementation - Fixed: simplified to avoid varargs issues
void printf(const char* format) {
    // Simple implementation that just prints the format string
    // For a full printf, you'd need to implement stdarg.h functionality
    terminal_writestring(format);
}

// Overloaded printf functions for different argument types
void printf_str(const char* format, const char* str) {
    const char* p = format;
    while (*p) {
        if (*p == '%' && *(p + 1) == 's') {
            terminal_writestring(str);
            p += 2;
        } else {
            terminal_putchar(*p);
            p++;
        }
    }
}

void printf_int(const char* format, int value) {
    const char* p = format;
    while (*p) {
        if (*p == '%' && (*(p + 1) == 'd' || *(p + 1) == 'x')) {
            // Simple integer to string conversion
            if (value == 0) {
                terminal_putchar('0');
            } else {
                char buffer[32];
                int i = 0;
                int temp = value;
                int is_negative = 0;
                
                if (temp < 0) {
                    is_negative = 1;
                    temp = -temp;
                }
                
                // Convert to string
                while (temp > 0) {
                    if (*(p + 1) == 'x') {
                        // Hexadecimal
                        int digit = temp % 16;
                        if (digit < 10) {
                            buffer[i++] = '0' + digit;
                        } else {
                            buffer[i++] = 'a' + (digit - 10);
                        }
                        temp /= 16;
                    } else {
                        // Decimal
                        buffer[i++] = '0' + (temp % 10);
                        temp /= 10;
                    }
                }
                
                if (is_negative) {
                    buffer[i++] = '-';
                }
                
                // Print in reverse order
                for (int j = i - 1; j >= 0; j--) {
                    terminal_putchar(buffer[j]);
                }
            }
            p += 2;
        } else {
            terminal_putchar(*p);
            p++;
        }
    }
}

// Simple memory allocator
void* kmalloc(size_t size) {
    if (heap_current + size > heap_end) {
        return NULL;  // Out of memory
    }
    
    void* ptr = heap_current;
    heap_current += size;
    
    // Align to 4-byte boundary
    if ((uintptr_t)heap_current % 4 != 0) {
        heap_current += 4 - ((uintptr_t)heap_current % 4);
    }
    
    return ptr;
}

void kfree(void* ptr) {
    // Simple allocator doesn't support freeing individual blocks
    // In a real OS, you'd implement a proper memory manager
    (void)ptr;  // Suppress unused parameter warning
}

// Memory information functions
void print_memory_info(void) {
    printf("Memory Information:\n");
    printf("Heap start: 0x");
    printf_int("%x", (int)(uintptr_t)heap_start);
    printf("\n");
    printf("Heap end: 0x");
    printf_int("%x", (int)(uintptr_t)heap_end);
    printf("\n");
    printf("Heap current: 0x");
    printf_int("%x", (int)(uintptr_t)heap_current);
    printf("\n");
    printf("Available memory: ");
    printf_int("%d", (int)(heap_end - heap_current));
    printf(" bytes\n");
}

// Kernel panic function
void kernel_panic(const char* message) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    printf("\nKERNEL PANIC: ");
    printf(message);
    printf("\nSystem halted.\n");
    
    // Halt the system
    while (1) {
        asm volatile("hlt");
    }
}

// Stub implementations for missing functions
void init_interrupts(void) {
    // Placeholder for interrupt initialization
    printf("Interrupt system initialized.\n");
}

void init_shell(void) {
    // Placeholder for shell initialization
    printf("Shell initialized.\n");
}

// Main kernel entry point - Fixed: removed syntax errors
void kernel_main(void) {
    // Initialize terminal
    terminal_initialize();
    
    // Print welcome message
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    printf("Welcome to MyOS!\n");
    printf("================\n\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    printf("Kernel loaded successfully!\n");
    printf("Terminal initialized.\n");
    printf("Memory allocator ready.\n");
    
    // Initialize interrupt system
    init_interrupts();
    
    // Test memory allocator
    printf("Testing memory allocator...\n");
    void* test_ptr = kmalloc(100);
    if (test_ptr) {
        printf("Successfully allocated 100 bytes at address: 0x");
        printf_int("%x", (int)(uintptr_t)test_ptr);
        printf("\n");
    } else {
        printf("Failed to allocate memory!\n");
    }
    
    // Show memory information
    print_memory_info();
    
    printf("\nKernel initialization complete!\n");
    
    // Initialize and start shell
    init_shell();
    
    printf("Kernel is ready.\n");
    
    // Keep the kernel running
    printf("\nKernel is now running. Press Ctrl+Alt+Del to restart.\n");
    while (1) {
        asm volatile("hlt");  // Halt until next interrupt
    }
}