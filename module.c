#include <ygg/module.h>
#include <drivers/pci/pci.h>
#include <sys/debug.h>
#include <stdint.h>

MODULE_DESC("test-mod", 0x00010000);

extern uint64_t system_time;

static void pci_driver(struct pci_device *dev) {
    kinfo("Loaded the driver!\n");
}

void MODULE_EXIT(void) {
    kinfo("Exiting the module!\n");
}

int MODULE_ENTRY(void) {
    kinfo("This should work, I guess: %s, %d\n", "a string", 123);

    pci_add_device_driver(PCI_ID(0x8086, 0x2930), pci_driver, "test-mod");
    return 0;
}
