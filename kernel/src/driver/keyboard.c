#include <driver/keyboard.h>
#include <stdint.h>
#include <io.h>
#include <sys/idt.h>
#include <sys/pic.h>
#include <proc/sched.h>

#define BUFFER_SIZE 256

char buffer[BUFFER_SIZE];
int buffer_head = 0;
int buffer_tail = 0;
int shift_pressed = 0;
int ctrl_pressed = 0;
int alt_pressed = 0;

// ASCII mappings for normal keys
const char* scancodeToASCII[256] = {
    "", "\x1B", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "\b",
    "\t", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "\n",
    "", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "`",
    "", "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "",
    "", "", " ", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
};

// ASCII mappings with Shift pressed
const char* scancodeToASCII_shift[256] = {
    "", "\x1B", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "\b",
    "\t", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "\n",
    "", "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "~",
    "", "|", "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "",
    "", "", " ", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
};

// ASCII mappings with Ctrl pressed (where applicable)
const char* scancodeToASCII_ctrl[256] = {
    "", "\x1B", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "\x11", "\x17", "\x05", "\x12", "\x14", "\x19", "\x15", "\x09", "\x0F", "\x10", "", "", "\n",
    "", "\x01", "\x13", "\x04", "\x06", "\x07", "\x08", "\x0A", "\x0B", "\x0C", "", "", "",
    "", "", "\x1A", "\x18", "\x03", "\x16", "\x02", "\x0E", "\x0D", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
};


void keyboardIRQHandler(registers_t*)
{
    uint8_t scancode = inb(0x60);

    if (scancode & 0x80) {
        scancode &= 0x7F;
        if (scancode == 0x2A || scancode == 0x36) shift_pressed = 0;
        else if (scancode == 0x1D) ctrl_pressed = 0;
        else if (scancode == 0x38) alt_pressed = 0;
    } else {
        if (scancode == 0x2A || scancode == 0x36) shift_pressed = 1;
        else if (scancode == 0x1D) ctrl_pressed = 1;
        else if (scancode == 0x38) alt_pressed = 1;
        else {
            const char* key;
            if (shift_pressed) key = scancodeToASCII_shift[scancode];
            else if (ctrl_pressed) key = scancodeToASCII_ctrl[scancode];
            else key = scancodeToASCII[scancode];

            char c = key[0];
            if (c && ((buffer_head + 1) % BUFFER_SIZE != buffer_tail)) {
                buffer[buffer_head] = c;
                buffer_head = (buffer_head + 1) % BUFFER_SIZE;
            }
        }
    }
    pic_sendeoi(1);
}

void keyboard_init()
{
    idt_register_handler(33, keyboardIRQHandler);
}

char keyboard_getchar()
{
    if (buffer_head == buffer_tail) return -1;
    char c = buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
    return c;
}

void keyboard_clear_buffer()
{
    buffer_head = buffer_tail = 0;
}

int keyboard_buffer_size()
{
    return (buffer_head + BUFFER_SIZE - buffer_tail) % BUFFER_SIZE;
}

int keyboard_key_pressed()
{
    return buffer_head != buffer_tail;
}

size_t keyboard_gets(char* out_buffer, size_t max_length)
{
    size_t i = 0;
    while (i < max_length - 1) {
        char c = keyboard_getchar();
        if (c == '\n' || c == -1) break;
        out_buffer[i++] = c;
    }
    out_buffer[i] = '\0';
    return i;
}

void keyboard_set_repeat_rate(uint8_t rate)
{
    outb(0x60, 0xF3);
    outb(0x60, rate);
}

void keyboard_reset()
{
    shift_pressed = ctrl_pressed = alt_pressed = 0;
    keyboard_clear_buffer();
}
