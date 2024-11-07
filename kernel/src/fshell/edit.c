#include <fshell/common.h>
#include <io.h>
#include <kprintf>
#include <proc/vfs.h>
#include <proc/ramfs.h>
#include <sys/pic.h>
#include <fshell/framebuffer.h>
#include <string.h>

#define BUFFER_SIZE 4096

void text_editor(const char* path) {
    HANDLE fileHandle = open(path);
    uint8_t* buffer = kmalloc(BUFFER_SIZE);
    uint32_t file_size = fileHandle->size;

    fileHandle->read(fileHandle, 0, file_size, buffer);

    cls();
    puts(" ------------ ");
    puts(path);
    puts(" ------------ \n");

    int cursor_x = 0, cursor_y = 0;
    uint32_t buffer_pos = 0;
    bool running = true;

    for (uint32_t i = 0; i < file_size; i++) {
        if (buffer[i] == '\n') {
            putc('\n');
            cursor_y += FONT_HEIGHT;
            cursor_x = 0;
        } else {
            putc(buffer[i]);
            cursor_x += FONT_WIDTH;
        }
    }

    while (running) {
        char key = getchar_locking();
        switch (key) {
            case 'w':
                if (cursor_y > 0) cursor_y--;
                break;
            case 's':
                cursor_y += FONT_HEIGHT;
                break;
            case 'a':
                if (cursor_x > 0) cursor_x--;
                break;
            case 'd':
                cursor_x += FONT_WIDTH;
                break;
            case '\n':
                if (buffer_pos < BUFFER_SIZE - 1) {
                    buffer[buffer_pos++] = '\n';
                    cursor_x = 0;
                    cursor_y += FONT_HEIGHT;
                }
                break;
            case '\x7F':
                if (buffer_pos > 0) {
                    buffer[--buffer_pos] = ' ';
                    cursor_x--;
                }
                break;
            case '\t':
                fileHandle->write(fileHandle, 0, buffer_pos, buffer);
                break;
            case '\x1B':
                running = false;
                break;
            default:
                if (buffer_pos < BUFFER_SIZE - 1) {
                    buffer[buffer_pos++] = key;
                    cursor_x += FONT_WIDTH;
                    putc(key);
                }
        }

        set_cursor(cursor_x, cursor_y);//todo
    }

    kfree(buffer);
    close(fileHandle);
}
