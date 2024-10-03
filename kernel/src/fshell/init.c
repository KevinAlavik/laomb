#include <fshell/init.h>
#include <fshell/common.h>
#include <sys/pic.h>

void init_fshell() {
    irq_register_handler(1, fshell_interrupt_handler);
    pic_mask(1);
}