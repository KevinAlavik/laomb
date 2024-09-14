#include <stdint.h>
#include <io.h>

#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif

static inline void puts(const char *str)
{
    while (*str)
    {
        outb( 0xe9, *str++ );
    }
}

    
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;
[[noreturn]] void __stack_chk_fail()
{
    puts("Stack smashing detected!\n");
	cli(); // TODO kpanic()
    for(;;) {
        hlt();
    }
	__builtin_unreachable();
}

[[noreturn]] void __stack_chk_fail_local() {
    __stack_chk_fail();
    __builtin_unreachable();
}