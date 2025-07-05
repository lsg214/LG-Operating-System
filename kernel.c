/* Complete kernel source code with all necessary definitions */
/* This kernel provides basic VGA text mode output and memory management */

/* Standard type definitions for freestanding environment */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uintptr_t;
typedef unsigned int size_t;

/* NULL pointer definition */
#define NULL ((void*)0)

/* VGA text mode constants */
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

/* VGA color constants */
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
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
};

/* Global terminal state variables */
static uint16_t* terminal_buffer;
static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;

/* Simple heap allocator variables */
static uint8_t* heap_start = (uint8_t*)0x200000;  /* Start heap at 2MB */
static uint8_t* heap_end = (uint8_t*)0x400000;    /* End heap at 4MB */
static uint8_t* heap_current = (uint8_t*)0x200000;

/* Create VGA entry combining character and color */
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

/* Create VGA color from foreground and background colors */
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

/* Simple string length function */
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

/* Terminal initialization and control */
void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t*) VGA_MEMORY;
    
    /* Clear the screen */
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
    /* Move all lines up by one */
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = terminal_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    /* Clear the last line */
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

/* Simple printf implementation */
void printf(const char* format, ...) {
    /* Simple implementation that only handles %s, %d, %x, and %c */
    const char* p = format;
    
    /* We'll use a simple approach without va_args for now */
    /* In a real implementation, you'd use stdarg.h */
    while (*p) {
        if (*p == '%' && *(p + 1)) {
            p++;
            switch (*p) {
                case 's': {
                    /* For this simple example, we'll just print the format */
                    terminal_writestring("[string]");
                    break;
                }
                case 'd': {
                    terminal_writestring("[number]");
                    break;
                }
                case 'x': {
                    terminal_writestring("[hex]");
                    break;
                }
                case 'c': {
                    terminal_writestring("[char]");
                    break;
                }
                default:
                    terminal_putchar('%');
                    terminal_putchar(*p);
                    break;
            }
        } else {
            terminal_putchar(*p);
        }
        p++;
    }
}

/* Simple memory allocator */
void* kmalloc(size_t size) {
    if (heap_current + size > heap_end) {
        return NULL;  /* Out of memory */
    }
    
    void* ptr = heap_current;
    heap_current += size;
    
    /* Align to 4-byte boundary */
    if ((uintptr_t)heap_current % 4 != 0) {
        heap_current += 4 - ((uintptr_t)heap_current % 4);
    }
    
    return ptr;
}

void kfree(void* ptr) {
    /* Simple allocator doesn't support freeing individual blocks */
    /* In a real OS, you'd implement a proper memory manager */
    (void)ptr;  /* Suppress unused parameter warning */
}

/* Memory information functions */
void print_memory_info(void) {
    printf("Memory Information:\n");
    printf("Heap start: 0x%x\n", (uintptr_t)heap_start);
    printf("Heap end: 0x%x\n", (uintptr_t)heap_end);
    printf("Heap current: 0x%x\n", (uintptr_t)heap_current);
    printf("Available memory: %d bytes\n", (size_t)(heap_end - heap_current));
}

/* Kernel panic function */
void kernel_panic(const char* message) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    printf("\nKERNEL PANIC: %s\n", message);
    printf("System halted.\n");
    
    /* Halt the system */
    while (1) {
        asm volatile("hlt");
    }
}

/* Main kernel entry point */
void kernel_main(void) {
    /* Initialize terminal */
    terminal_initialize();
    
    /* Print welcome message */
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    printf("Welcome to MyOS!\n");
    printf("================\n\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    printf("Kernel loaded successfully!\n");
    printf("Terminal initialized.\n");
    printf("Memory allocator ready.\n\n");
    
    /* Test memory allocator */
    printf("Testing memory allocator...\n");
    void* test_ptr = kmalloc(100);
    if (test_ptr) {
        printf("Successfully allocated 100 bytes at address: 0x%x\n", (uintptr_t)test_ptr);
    } else {
        printf("Failed to allocate memory!\n");
    }
    
    /* Show memory information */
    print_memory_info();
    
    /* Test different colors */
    printf("\nTesting colors:\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
    printf("Red text\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    printf("Green text\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_BLUE, VGA_COLOR_BLACK));
    printf("Blue text\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
    printf("Yellow text\n");
    
    /* Reset to normal color */
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    printf("\nKernel initialization complete!\n");
    printf("System is ready.\n");
    
    /* Keep the kernel running */
    printf("\nKernel is now running. Press Ctrl+Alt+Del to restart.\n");
    while (1) {
        asm volatile("hlt");  /* Halt until next interrupt */
    }
}