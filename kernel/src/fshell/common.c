#include <fshell/common.h>
#include <io.h>
#include <kprintf>
#include <proc/vfs.h>
#include <proc/ramfs.h>
#include <sys/pic.h>
#include <fshell/framebuffer.h>

struct fshell_ctx fshell_ctx;

const char* scancodeToASCII[256] = {
    "", "\x1B", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "\x7F",
    "\t", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "\n",
    "", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "`",
    "", "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "",
    "", "", " ", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", ""
};

const char* scancodeToASCII_shift[256] = {
    "", "\x1B", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "\x7F",
    "\t", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "\n",
    "", "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "~",
    "", "|", "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "",
    "", " ", "", "", "", "", "", "", "", "", "", "", "", "", ""
};

char getchar_locking() {
    while (fshell_ctx.count == 0)
        yield();
    char c = fshell_ctx.buffer[(fshell_ctx.buffer_index - fshell_ctx.count + FSHELL_BUFFER_SIZE) % FSHELL_BUFFER_SIZE];
    fshell_ctx.count--;
    return c;
}

void fshell_interrupt_handler(registers_t* ) {
    uint8_t scancode = inb(0x60);
    if (scancode & 0x80) {
        scancode &= 0x7F;
        if (scancode == 0x2A || scancode == 0x36) {
            fshell_ctx.shift_pressed = 0;
        } else if (scancode == 0x1D) {
            fshell_ctx.ctrl_pressed = 0;
        }
    } else {
        if (scancode == 0x2A || scancode == 0x36) {
            fshell_ctx.shift_pressed = 1;
        } else if (scancode == 0x1D) {
            fshell_ctx.ctrl_pressed = 1;
        } else {
            const char* key = fshell_ctx.shift_pressed ? scancodeToASCII_shift[scancode] : scancodeToASCII[scancode];
            char c = key[0];
            if (c != '\0' && fshell_ctx.count < FSHELL_BUFFER_SIZE) {
                fshell_ctx.buffer[fshell_ctx.buffer_index++] = c;
                fshell_ctx.count++;
                fshell_ctx.buffer_index %= FSHELL_BUFFER_SIZE;
            }
        }
    }
    pic_sendeoi(1);
}

static void trim_whitespace(char *str) {
    char *start = str;
    char *end;

    while (isspace((unsigned char)*start)) start++;
    
    if (*start == '\0') {
        str[0] = '\0';
        return;
    }
    memmove(str, start, strlen(start) + 1);
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    *(end + 1) = '\0';
}


static void resolve_path(char *resolved_path, const char *path) {
    if (resolved_path == NULL) return;
    if (path == NULL || strlen(path) == 0) {
        strcpy(resolved_path, fshell_ctx.pwd);
    } else {
        strcpy(resolved_path, path);
    }
    trim_whitespace(resolved_path);

    if (resolved_path[0] != '/') {
        char temp[FSHELL_BUFFER_SIZE];
        strcpy(temp, fshell_ctx.pwd);
        strcat(temp, "/");
        strcat(temp, resolved_path);
        strcpy(resolved_path, temp);
    }

    char temp_path[FSHELL_BUFFER_SIZE];
    char *token, *components[FSHELL_BUFFER_SIZE];
    int component_count = 0;

    strcpy(temp_path, resolved_path);
    token = strtok(temp_path, "/");
    while (token != NULL) {
        if (strcmp(token, ".") == 0) {
        } else if (strcmp(token, "..") == 0) {
            if (component_count > 0) component_count--;
        } else {
            components[component_count++] = token;
        }
        token = strtok(NULL, "/");
    }

    resolved_path[0] = '/';
    resolved_path[1] = '\0';
    for (int i = 0; i < component_count; i++) {
        strcat(resolved_path, components[i]);
        if (i < component_count - 1) strcat(resolved_path, "/");
    }
}

#define COMMAND_COUNT 10

typedef struct {
    const char *name;
    void (*function)(char *args);
} command_t;

static void command_help(char *args);
static void command_echo(char *args);
static void command_clear(char *args);
static void command_ls(char *args);
static void command_mkdir(char *args);
static void command_cd(char *args);
static void command_cat(char *args);
static void command_touch(char *args);
static void command_rm(char *args);
static void command_rmdir(char *args);

command_t commands[COMMAND_COUNT] = {
    {"help", command_help},
    {"echo", command_echo},
    {"clear", command_clear},
    {"ls", command_ls},
    {"cat", command_cat},
    {"mkdir", command_mkdir},
    {"cd", command_cd},
    {"touch", command_touch},
    {"rm", command_rm},
    {"rmdir", command_rmdir}
};

static HANDLE handle_redirection(char *args, int *write_mode) {
    if (args == NULL) return NULL;

    char *redir_pos = strstr(args, ">");
    if (redir_pos == NULL) return NULL;

    *write_mode = WRITE_MODE_TRUNCATE;

    if (*(redir_pos + 1) == '>') {
        *write_mode = WRITE_MODE_APPEND;
        *(redir_pos++) = '\0';
    }

    *(redir_pos++) = '\0';

    while (*redir_pos == ' ') redir_pos++;

    char full_path[FSHELL_BUFFER_SIZE];
    resolve_path(full_path, redir_pos);

    HANDLE handle = open(full_path);
    if (!handle) {
        vfs_err_t err = create(full_path, VFS_RAMFS_FILE);
        if (err != VFS_SUCCESS) {
            puts("Failed to create file for redirection.\n");
            return NULL;
        }
        handle = open(full_path);
    }

    return handle;
}


void command_help(char *args) {
    int write_mode = WRITE_MODE_TRUNCATE;
    HANDLE output_handle = handle_redirection(args, &write_mode);
    int offset = (write_mode == WRITE_MODE_APPEND && output_handle) ? output_handle->size : 0;

    for (int i = 0; i < COMMAND_COUNT; i++) {
        if (output_handle) {
            write(output_handle, offset, strlen(commands[i].name), (uint8_t *)commands[i].name);
            offset += strlen(commands[i].name);
            write(output_handle, offset, 1, (uint8_t *)"\n");
            offset += 1;
        } else {
            puts(commands[i].name);
            puts("\n");
        }
    }

    if (output_handle) close(output_handle);
}


void command_echo(char *args) {
    if (args == NULL || strlen(args) == 0) {
        puts("Usage: echo <message>\n");
        return;
    }

    int write_mode = WRITE_MODE_TRUNCATE;
    HANDLE output_handle = handle_redirection(args, &write_mode);
    int offset = (write_mode == WRITE_MODE_APPEND && output_handle) ? output_handle->size : 0;

    if (output_handle) {
        write(output_handle, offset, strlen(args), (uint8_t *)args);
        offset += strlen(args);
        write(output_handle, offset, 1, (uint8_t *)"\n");
        close(output_handle);
    } else {
        puts(args);
        puts("\n");
    }
}

void command_clear(char *) {
    cls();
}

void command_cat(char *args) {
    if (args == NULL || strlen(args) == 0) {
        puts("Usage: cat <file>\n");
        return;
    }

    int write_mode = WRITE_MODE_TRUNCATE;
    HANDLE output_handle = handle_redirection(args, &write_mode);
    int offset = (write_mode == WRITE_MODE_APPEND && output_handle) ? output_handle->size : 0;

    char full_path[FSHELL_BUFFER_SIZE];
    resolve_path(full_path, args);

    HANDLE handle = open(full_path);
    if (!handle) {
        if (!output_handle) puts("File not found.\n");
        else write(output_handle, offset, 15, (uint8_t *)"File not found.\n");
        return;
    }

    if (handle->type != VFS_RAMFS_FILE) {
        if (!output_handle) puts("Not a file.\n");
        else write(output_handle, offset, 11, (uint8_t *)"Not a file.\n");
        close(handle);
        return;
    }

    uint8_t buffer[FSHELL_BUFFER_SIZE];
    vfs_read(handle, 0, handle->size, buffer);
    if (output_handle) {
        write(output_handle, offset, handle->size, buffer);
        close(output_handle);
    } else {
        puts((char *)buffer);
        puts("\n");
    }

    close(handle);
}

void command_touch(char *args) {
    if (args == NULL || strlen(args) == 0) {
        puts("Usage: touch <file>\n");
        return;
    }

    char full_path[FSHELL_BUFFER_SIZE];
    resolve_path(full_path, args);

    HANDLE handle = open(full_path);
    if (handle) {
        close(handle);
        return;
    }

    vfs_err_t err = create(full_path, VFS_RAMFS_FILE);
    if (err == VFS_NOT_FOUND) {
        puts("Directory doesn't exist.\n");
    } else if (err == VFS_ERROR) {
        puts("Failed to create file.\n");
    } else if (err == VFS_NOT_PERMITTED) {
        puts("Cannot create file in a special directory.\n");
    }
}

void command_rm(char *args) {
    if (args == NULL || strlen(args) == 0) {
        puts("Usage: rm <file>\n");
        return;
    }

    char full_path[FSHELL_BUFFER_SIZE];
    resolve_path(full_path, args);

    HANDLE handle = open(full_path);
    if (!handle) {
        puts("File not found.\n");
        return;
    }

    if (handle->type != VFS_RAMFS_FILE) {
        puts("Not a file.\n");
        close(handle);
        return;
    }

    vfs_err_t err = remove(handle);
    if (err != VFS_SUCCESS) {
        puts("Failed to remove file.\n");
    } else {
        puts("File removed successfully.\n");
    }

    close(handle);
}

void command_rmdir(char *args) {
    if (args == NULL || strlen(args) == 0) {
        puts("Usage: rmdir <directory>\n");
        return;
    }

    char full_path[FSHELL_BUFFER_SIZE];
    resolve_path(full_path, args);

    HANDLE handle = open(full_path);
    if (!handle) {
        puts("Directory not found.\n");
        return;
    }

    if (handle->type != VFS_RAMFS_FOLDER) {
        puts("Not a directory.\n");
        close(handle);
        return;
    }

    if (handle->children) {
        puts("Directory is not empty.\n");
        close(handle);
        return;
    }

    vfs_err_t err = remove(handle);
    if (err != VFS_SUCCESS) {
        puts("Failed to remove directory.\n");
    } else {
        puts("Directory removed successfully.\n");
    }

    close(handle);
}

void command_ls(char *args) {
    int write_mode = WRITE_MODE_TRUNCATE;
    HANDLE output_handle = handle_redirection(args, &write_mode);
    int offset = (write_mode == WRITE_MODE_APPEND && output_handle) ? output_handle->size : 0;

    char full_path[FSHELL_BUFFER_SIZE];
    resolve_path(full_path, args ? args : fshell_ctx.pwd);

    HANDLE handle = open(full_path);
    if (!handle) {
        if (!output_handle) puts("Directory not found.\n");
        else write(output_handle, offset, 17, (uint8_t *)"Directory not found.\n");
        return;
    }

    if (handle->type != VFS_RAMFS_FOLDER) {
        if (!output_handle) puts("Not a directory.\n");
        else write(output_handle, offset, 16, (uint8_t *)"Not a directory.\n");
        close(handle);
        return;
    }

    struct vfs_node *child = handle->children;
    while (child) {
        char *name = child->name;
        if (child->type == VFS_RAMFS_FOLDER) set_foreground_color(0xBB7799);
        if (output_handle) {
            write(output_handle, offset, strlen(name), (uint8_t *)name);
            offset += strlen(name);
            write(output_handle, offset, 1, (uint8_t *)" ");
            offset += 1;
        } else {
            puts(name);
            puts(" ");
        }
        if (child->type == VFS_RAMFS_FOLDER) set_foreground_color(0xFFFFFF);
        child = child->next;
    }
    if (output_handle) write(output_handle, offset, 1, (uint8_t *)"\n");
    else puts("\n");

    close(handle);
    if (output_handle) close(output_handle);
}

void command_mkdir(char *args) {
    if (args == NULL || strlen(args) == 0) {
        puts("Usage: mkdir <path>\n");
        return;
    }

    char full_path[FSHELL_BUFFER_SIZE];
    resolve_path(full_path, args);

    vfs_err_t err = create(full_path, VFS_RAMFS_FOLDER);
    if (err == VFS_NOT_FOUND) {
        puts("Folder doesn't exist.\n");
    } else if (err == VFS_ERROR) {
        puts("Failed to create folder.\n");
    } else if (err == VFS_NOT_PERMITTED) {
        puts("Cannot create folder in a special file.\n");
    }
}

static void command_cd(char *args) {
    if (args == NULL || strlen(args) == 0) {
        puts("Usage: cd <path>\n");
        return;
    }

    char full_path[FSHELL_BUFFER_SIZE];
    resolve_path(full_path, args);

    HANDLE handle = open(full_path);
    if (!handle) {
        puts("Directory not found.\n");
        return;
    }

    if (handle->type != VFS_RAMFS_FOLDER) {
        puts("Not a directory.\n");
        close(handle);
        return;
    }

    strcpy(fshell_ctx.pwd, full_path);
    close(handle);
}

extern void text_editor(const char* path);

void execute_command(const char *command, char *args) {
    if (command == NULL || strlen(command) == 0)
        return;

    for (int i = 0; i < COMMAND_COUNT; i++) {
        if (strcmp(command, commands[i].name) == 0) {
            commands[i].function(args);
            return;
        }
        if (strcmp(command, "edit") == 0) {
            char full_path[FSHELL_BUFFER_SIZE];
            resolve_path(full_path, args);
            text_editor(full_path);
            return;
        }
    }

    puts("Unknown command: ");
    puts(command);
    puts("\n");
}


void fshell_callback() {
    char buffer[FSHELL_BUFFER_SIZE];
    char command[64];
    char *args = NULL;
    int index = 0;
    strcpy(fshell_ctx.pwd, "/");

    pic_unmask(1);
    cls();
    puts(fshell_ctx.pwd);
    puts("> ");

    inb(0x60);
    while (1) {
        char c = getchar_locking();

        if (c == -1) {
            continue;
        }

        if (c == '\n') {
            buffer[index] = '\0';
            puts("\n");

            args = NULL;
            char *space = strchr(buffer, ' ');
            if (space) {
                *space = '\0';
                args = space + 1;
            }

            strcpy(command, buffer);
            if (strlen(command) > 0) {
                execute_command(command, args);
            }

            puts(fshell_ctx.pwd);
            puts("> ");
            index = 0;
        } else if (c == '\x7F') {
            if (index <= 0) {
                continue;
            }
            buffer[--index] = '\0';
            putc(c);
        } else {
            if (index < FSHELL_BUFFER_SIZE - 1) {
                buffer[index++] = c;
                putc(c);
            }
        }
    }

    puts("Shell terminated\n");
    sched_remove_task(current_task->pid);
}
