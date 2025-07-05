/*
 * interrupts.c - Interrupt handling system
 * Handles CPU interrupts including keyboard input
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

// Interrupt constants
#define IDT_SIZE 256
#define KEYBOARD_IRQ 1
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20

// Port I/O functions
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// IDT Entry structure
struct idt_entry {
    uint16_t offset_low;    // Lower 16 bits of handler address
    uint16_t selector;      // Kernel segment selector
    uint8_t zero;           // Always zero
    uint8_t type_attr;      // Type and attributes
    uint16_t offset_high;   // Upper 16 bits of handler address
} __attribute__((packed));

// IDT Pointer structure
struct idt_ptr {
    uint16_t limit;         // Size of IDT
    uint32_t base;          // Base address of IDT
} __attribute__((packed));

// Global IDT
static struct idt_entry idt[IDT_SIZE];
static struct idt_ptr idtp;

// Keyboard scancode to ASCII mapping
static char scancode_to_ascii[] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

// Keyboard state
static int shift_pressed = 0;
static int caps_lock = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;

// Input buffer
#define INPUT_BUFFER_SIZE 256
static char input_buffer[INPUT_BUFFER_SIZE];
static int input_index = 0;

// External functions (from kernel.c)
extern void terminal_putchar(char c);
extern void terminal_writestring(const char* str);
extern void printf(const char* format, ...);

// Forward declarations
void keyboard_handler(void);
void handle_keyboard_input(uint8_t scancode);

// Set an IDT entry
void set_idt_entry(int num, uint32_t handler, uint16_t selector, uint8_t flags) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
    idt[num].offset_high = (handler >> 16) & 0xFFFF;
}

// Initialize the PIC (Programmable Interrupt Controller)
void init_pic(void) {
    // Save masks
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    
    // Start initialization sequence
    outb(PIC1_COMMAND, 0x11);  // ICW1: Initialize
    outb(PIC2_COMMAND, 0x11);
    
    // ICW2: Set interrupt vector offsets
    outb(PIC1_DATA, 0x20);     // PIC1 starts at interrupt 32
    outb(PIC2_DATA, 0x28);     // PIC2 starts at interrupt 40
    
    // ICW3: Set up cascading
    outb(PIC1_DATA, 0x04);     // PIC1 has slave at IRQ2
    outb(PIC2_DATA, 0x02);     // PIC2 cascade identity
    
    // ICW4: Set mode
    outb(PIC1_DATA, 0x01);     // 8086 mode
    outb(PIC2_DATA, 0x01);
    
    // Restore masks (disable all interrupts initially)
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    
    // Enable keyboard interrupt (IRQ1)
    outb(PIC1_DATA, 0xFD);     // Enable IRQ1 (keyboard)
}

// Send End of Interrupt signal
void send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

// Assembly wrapper for keyboard interrupt
asm(
    ".global keyboard_interrupt_wrapper\n"
    "keyboard_interrupt_wrapper:\n"
    "    pusha\n"                    // Save all registers
    "    call keyboard_handler\n"    // Call C handler
    "    popa\n"                     // Restore registers
    "    iret\n"                     // Return from interrupt
);

// External declaration for assembly wrapper
extern void keyboard_interrupt_wrapper(void);

// Initialize IDT
void init_idt(void) {
    // Set up IDT pointer
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)&idt;
    
    // Clear IDT
    for (int i = 0; i < IDT_SIZE; i++) {
        set_idt_entry(i, 0, 0, 0);
    }
    
    // Set keyboard interrupt handler (IRQ1 = interrupt 33)
    set_idt_entry(33, (uint32_t)keyboard_interrupt_wrapper, 0x08, 0x8E);
    
    // Load IDT
    asm volatile("lidt %0" : : "m"(idtp));
}

// Initialize interrupt system
void init_interrupts(void) {
    printf("Initializing interrupt system...\n");
    
    // Initialize IDT
    init_idt();
    
    // Initialize PIC
    init_pic();
    
    // Enable interrupts
    asm volatile("sti");
    
    printf("Interrupts enabled!\n");
}

// Keyboard interrupt handler
void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);  // Read scancode from keyboard port
    handle_keyboard_input(scancode);
    send_eoi(KEYBOARD_IRQ);        // Send End of Interrupt
}

// Convert character to uppercase
char to_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }
    return c;
}

// Convert character to lowercase
char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}

// Handle keyboard input
void handle_keyboard_input(uint8_t scancode) {
    // Check for key release (high bit set)
    if (scancode & 0x80) {
        // Key released
        scancode &= 0x7F;  // Remove release bit
        
        // Handle modifier key releases
        switch (scancode) {
            case 0x2A: case 0x36: // Shift
                shift_pressed = 0;
                break;
            case 0x1D: // Ctrl
                ctrl_pressed = 0;
                break;
            case 0x38: // Alt
                alt_pressed = 0;
                break;
        }
        return;
    }
    
    // Handle special keys
    switch (scancode) {
        case 0x2A: case 0x36: // Shift
            shift_pressed = 1;
            return;
        case 0x1D: // Ctrl
            ctrl_pressed = 1;
            return;
        case 0x38: // Alt
            alt_pressed = 1;
            return;
        case 0x3A: // Caps Lock
            caps_lock = !caps_lock;
            return;
        case 0x0E: // Backspace
            if (input_index > 0) {
                input_index--;
                input_buffer[input_index] = '\0';
                terminal_putchar('\b');
                terminal_putchar(' ');
                terminal_putchar('\b');
            }
            return;
        case 0x1C: // Enter
            input_buffer[input_index] = '\0';
            terminal_putchar('\n');
            process_command(input_buffer);
            input_index = 0;
            show_prompt();
            return;
    }
    
    // Convert scancode to ASCII
    if (scancode < sizeof(scancode_to_ascii)) {
        char c = scancode_to_ascii[scancode];
        if (c != 0) {
            // Handle shift and caps lock
            if (shift_pressed) {
                // Shift pressed - handle special characters
                switch (c) {
                    case '1': c = '!'; break;
                    case '2': c = '@'; break;
                    case '3': c = '#'; break;
                    case '4': c = '$'; break;
                    case '5': c = '%'; break;
                    case '6': c = '^'; break;
                    case '7': c = '&'; break;
                    case '8': c = '*'; break;
                    case '9': c = '('; break;
                    case '0': c = ')'; break;
                    case '-': c = '_'; break;
                    case '=': c = '+'; break;
                    case '[': c = '{'; break;
                    case ']': c = '}'; break;
                    case ';': c = ':'; break;
                    case '\'': c = '"'; break;
                    case '`': c = '~'; break;
                    case '\\': c = '|'; break;
                    case ',': c = '<'; break;
                    case '.': c = '>'; break;
                    case '/': c = '?'; break;
                    default:
                        if (c >= 'a' && c <= 'z') {
                            c = to_upper(c);
                        }
                        break;
                }
            } else if (caps_lock && c >= 'a' && c <= 'z') {
                c = to_upper(c);
            }
            
            // Handle Ctrl combinations
            if (ctrl_pressed) {
                switch (c) {
                    case 'c': case 'C':
                        terminal_writestring("^C\n");
                        input_index = 0;
                        show_prompt();
                        return;
                    case 'l': case 'L':
                        clear_screen();
                        show_prompt();
                        return;
                }
            }
            
            // Add character to input buffer
            if (input_index < INPUT_BUFFER_SIZE - 1) {
                input_buffer[input_index++] = c;
                terminal_putchar(c);
            }
        }
    }
}

// Get current input buffer
char* get_input_buffer(void) {
    return input_buffer;
}

// Clear input buffer
void clear_input_buffer(void) {
    input_index = 0;
    input_buffer[0] = '\0';
}

// Check if input buffer is empty
int is_input_empty(void) {
    return input_index == 0;
}