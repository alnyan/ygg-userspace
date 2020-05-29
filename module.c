#include <ygg/module.h>
#include <sys/debug.h>
#include <stdint.h>

MODULE_DESC("test-mod", 0x00010000);

extern uint64_t system_time;

void MODULE_EXIT(void) {
    kinfo("Exiting the module!\n");
}

int MODULE_ENTRY(void) {
    kinfo("This should work, I guess: %s, %d\n", "a string", 123);
    return 0;
}
