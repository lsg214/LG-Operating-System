/*
 * shell.c - Simple command-line shell
 * Provides basic commands and user interaction
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

// External functions
extern void terminal_putchar(char c);
extern void terminal_writestring(const char* str);
extern void printf(const char* format, ...);
extern void terminal_initialize(void);
extern void terminal_setcolor(uint8_t color);
extern void print_memory_info(void);
extern void* kmalloc(size_t size);
extern void kfree(void* ptr);
extern size_t strlen(const char* str);
extern void kernel_panic(const char* message);

// VGA colors
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_BLUE 1
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_CYAN 3
#define VGA_COLOR_RED 4
#define VGA_COLOR_MAGENTA 5
#define VGA_COLOR_BROWN 6
#define VGA_COLOR_LIGHT_GREY 7
#define VGA_COLOR_DARK_GREY 8
#define VGA_COLOR_LIGHT_BLUE 9
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_YELLOW 14
#define VGA_COLOR_WHITE 15

// Helper function to create VGA color
static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

// String comparison function
int strcmp(const char* str1, const char* str2) {
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            return *str1 - *str2;
        }
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

// String comparison function (case insensitive)
int strcmpi(const char* str1, const char* str2) {
    while (*str1 && *str2) {
        char c1 = *str1;
        char c2 = *str2;
        
        // Convert to lowercase
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        
        if (c1 != c2) {
            return c1 - c2;
        }
        str1++;
        str2++;
    }
    
    char c1 = *str1;
    char c2 = *str2;
    if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
    if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
    
    return c1 - c2;
}

// Check if string starts with prefix
int starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str != *prefix) {
            return 0;
        }
        str++;
        prefix++;
    }
    return 1;
}

// Simple tokenizer - splits command line into arguments
#define MAX_ARGS 16
static char* args[MAX_ARGS];
static int arg_count = 0;

void tokenize_command(char* input) {
    arg_count = 0;
    char* current = input;
    
    while (*current && arg_count < MAX_ARGS - 1) {
        // Skip whitespace
        while (*current == ' ' || *current == '\t') {
            current++;
        }
        
        if (*current == '\0') break;
        
        // Start of argument
        args[arg_count++] = current;
        
        // Find end of argument
        while (*current && *current != ' ' && *current != '\t') {
            current++;
        }
        
        // Null-terminate argument
        if (*current) {
            *current = '\0';
            current++;
        }
    }
    
    args[arg_count] = NULL;
}

// Built-in commands
void cmd_help(void) {
    printf("Available commands:\n");
    printf("  help      - Show this help message\n");
    printf("  clear     - Clear the screen\n");
    printf("  echo      - Echo arguments\n");
    printf("  meminfo   - Show memory information\n");
    printf("  memtest   - Test memory allocator\n");
    printf("  color     - Change text color\n");
    printf("  about     - Show system information\n");
    printf("  panic     - Trigger kernel panic (for testing)\n");
    printf("  reboot    - Reboot the system\n");
    printf("  shutdown  - Shutdown the system\n");
}

void cmd_clear(void) {
    terminal_initialize();
}

void cmd_echo(void) {
    for (int i = 1; i < arg_count; i++) {
        if (i > 1) printf(" ");
        printf("%s", args[i]);
    }
    printf("\n");
}

void cmd_meminfo(void) {
    print_memory_info();
}

void cmd_memtest(void) {
    printf("Testing memory allocator...\n");
    
    // Test 1: Basic allocation
    printf("Test 1: Basic allocation\n");
    void* ptr1 = kmalloc(100);
    if (ptr1) {
        printf("  Allocated 100 bytes at 0x%x\n", (uint32_t)ptr1);
    } else {
        printf("  Failed to allocate 100 bytes\n");
        return;
    }
    
    // Test 2: Multiple allocations
    printf("Test 2: Multiple allocations\n");
    void* ptr2 = kmalloc(50);
    void* ptr3 = kmalloc(200);
    void* ptr4 = kmalloc(75);
    
    if (ptr2 && ptr3 && ptr4) {
        printf("  Allocated multiple blocks successfully\n");
        printf("  Block 1: 0x%x, Block 2: 0x%x, Block 3: 0x%x\n", 
               (uint32_t)ptr2, (uint32_t)ptr3, (uint32_t)ptr4);
    } else {
        printf("  Failed to allocate multiple blocks\n");
    }
    
    // Test 3: Large allocation
    printf("Test 3: Large allocation\n");
    void* ptr5 = kmalloc(1024);
    if (ptr5) {
        printf("  Allocated 1024 bytes at 0x%x\n", (uint32_t)ptr5);
    } else {
        printf("  Failed to allocate 1024 bytes\n");
    }
    
    printf("Memory test completed!\n");
}

void cmd_color(void) {
    if (arg_count < 2) {
        printf("Usage: color <color_name>\n");
        printf("Available colors: red, green, blue, yellow, cyan, magenta, white, grey\n");
        return;
    }
    
    uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    if (strcmpi(args[1], "red") == 0) {
        color = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    } else if (strcmpi(args[1], "green") == 0) {
        color = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    } else if (strcmpi(args[1], "blue") == 0) {
        color = vga_entry_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    } else if (strcmpi(args[1], "yellow") == 0) {
        color = vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    } else if (strcmpi(args[1], "cyan") == 0) {
        color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    } else if (strcmpi(args[1], "magenta") == 0) {
        color = vga_entry_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    } else if (strcmpi(args[1], "white") == 0) {
        color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else if (strcmpi(args[1], "grey") == 0) {
        color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    } else {
        printf("Unknown color: %s\n", args[1]);
        return;
    }
    
    terminal_setcolor(color);
    printf("Color changed to %s\n", args[1]);
}

void cmd_about(void) {
    printf("MyOS - A Simple Operating System\n");
    printf("Version: 0.1.0\n");
    printf("Author: OS Developer\n");
    printf("Built: %s %s\n", __DATE__, __TIME__);
    printf("\nFeatures:\n");
    printf("  - VGA text mode display\n");
    printf("  - Keyboard input handling\n");
    printf("  - Basic memory management\n");
    printf("  - Simple command shell\n");
    printf("  - Interrupt handling\n");
    printf("\nThis is a minimal kernel for educational purposes.\n");
}

void cmd_panic(void) {
    if (arg_count > 1) {
        kernel_panic(args[1]);
    } else {
        kernel_panic("User-requested panic for testing");
    }
}

void cmd_reboot(void) {
    printf("Rebooting system...\n");
    // Wait a moment
    for (volatile int i = 0; i < 10000000; i++);
    
    // Reboot via keyboard controller
    uint8_t good = 0x02;
    while (good & 0x02) {
        good = inb(0x64);
    }
    outb(0x64, 0xFE);
    
    // If that fails, try triple fault
    asm volatile("int $0x00");
}

void cmd_shutdown(void) {
    printf("Shutting down system...\n");
    printf("It's now safe to power off your computer.\n");
    
    // Halt the CPU
    asm volatile("cli");
    while (1) {
        asm volatile("hlt");
    }
}

// Port I/O functions (needed for reboot)
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Show command prompt
void show_prompt(void) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    printf("MyOS");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    printf("$ ");
}

// Clear screen (called from interrupt handler)
void clear_screen(void) {
    terminal_initialize();
}

// Process a command
void process_command(char* input) {
    // Skip empty commands
    if (!input || *input == '\0') {
        return;
    }
    
    // Remove trailing whitespace
    char* end = input + strlen(input) - 1;
    while (end > input && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    
    // Skip empty commands after trimming
    if (*input == '\0') {
        return;
    }
    
    // Tokenize the command
    tokenize_command(input);
    
    if (arg_count == 0) {
        return;
    }
    
    // Execute built-in commands
    if (strcmpi(args[0], "help") == 0) {
        cmd_help();
    } else if (strcmpi(args[0], "clear") == 0) {
        cmd_clear();
    } else if (strcmpi(args[0], "echo") == 0) {
        cmd_echo();
    } else if (strcmpi(args[0], "meminfo") == 0) {
        cmd_meminfo();
    } else if (strcmpi(args[0], "memtest") == 0) {
        cmd_memtest();
    } else if (strcmpi(args[0], "color") == 0) {
        cmd_color();
    } else if (strcmpi(args[0], "about") == 0) {
        cmd_about();
    } else if (strcmpi(args[0], "panic") == 0) {
        cmd_panic();
    } else if (strcmpi(args[0], "reboot") == 0) {
        cmd_reboot();
    } else if (strcmpi(args[0], "shutdown") == 0) {
        cmd_shutdown();
    } else {
        printf("Unknown command: %s\n", args[0]);
        printf("Type 'help' for available commands.\n");
    }
}

// Initialize shell
void init_shell(void) {
    printf("\nWelcome to MyOS Shell!\n");
    printf("Type 'help' for available commands.\n\n");
    show_prompt();
}