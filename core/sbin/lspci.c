#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

struct pci_device_config {
    uint16_t id_vendor;
    uint16_t id_device;
    uint8_t id_class, id_subclass, id_prog_if;
};

static int read_pci_device(struct pci_device_config *cfg, FILE *fp) {
#define FIELD_PARSE(prefix, fmt, field) \
    if (!strncmp(buf, (prefix), sizeof(prefix) - 1)) { \
        p = buf + sizeof(prefix) - 1; \
        while (isspace(*p)) ++p; \
        sscanf(p, fmt, (uint16_t *) &cfg->field); \
    }
    char buf[512];
    const char *p;

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        FIELD_PARSE("vendor ", "%hx", id_vendor) else
        FIELD_PARSE("device ", "%hx", id_device) else
        FIELD_PARSE("class ", "%hx", id_class) else
        FIELD_PARSE("subclass ", "%hx", id_subclass) else
        FIELD_PARSE("prog_if ", "%hx", id_prog_if)
    }

#undef FIELD_PARSE
    return 0;
}

static const char *pci_device_type(struct pci_device_config *dev) {
    switch (dev->id_class) {
    case 0x01:
        switch (dev->id_subclass) {
        case 0x00: return "SCSI Bus Controller";
        case 0x01: return "IDE Controller";
        case 0x02: return "Floppy Disk Controller";
        case 0x03: return "IPI Bus Controller";
        case 0x04: return "RAID Controller";
        case 0x05: return "ATA Controller";
        case 0x06: return "SATA Controller";
        case 0x07: return "SAS Controller";
        case 0x08: return "NVM Controller";
        default: return "Other Mass Storage";
        }
    case 0x02:
        switch (dev->id_subclass) {
        case 0x00: return "Ethernet Controller";
        case 0x01: return "Token Ring Controller";
        case 0x02: return "FDDI Controller";
        case 0x03: return "ATM Controller";
        case 0x04: return "ISDN Controller";
        case 0x05: return "WorldFip Controller";
        case 0x06: return "PICMG 2.14 Multi Computing";
        case 0x07: return "Inifiniband Controller";
        case 0x08: return "Fabric Controller";
        default: return "Other Network Device";
        }
    case 0x03:
        switch (dev->id_subclass) {
        case 0x00: return "VGA Compatible Controller";
        case 0x01: return "XGA Controller";
        case 0x02: return "3D Controller (Not VGA-Compatible)";
        default: return "Other Display Controller";
        }
    case 0x04: return "Multimedia Controller";
    case 0x05: return "Memory Controller";
    case 0x06:
        switch (dev->id_subclass) {
        case 0x00: return "Host Bridge";
        case 0x01: return "ISA Bridge";
        case 0x02: return "EISA Bridge";
        case 0x03: return "MCA Bridge";
        case 0x04: return "PCI-to-PCI Bridge";
        case 0x05: return "PCMCIA Bridge";
        case 0x06: return "NuBus Bridge";
        case 0x07: return "CardBus Bridge";
        case 0x08: return "RACEway Bridge";
        case 0x09: return "PCI-to-PCI Bridge";
        case 0x0A: return "InfiniBand-to-PCI Host Bridge";
        default: return "Other Bridge";
        }
    case 0x07:
        switch (dev->id_subclass) {
        case 0x00: return "Serial Controller";
        case 0x01: return "Parallel Controller";
        case 0x02: return "Multiport Serial Controller";
        case 0x03: return "Modem";
        case 0x04: return "IEEE 488.1/2 (GPIB) Controller";
        case 0x05: return "Smart Card";
        default: return "Other Simple Communications";
        }
    case 0x08:
        switch (dev->id_subclass) {
        case 0x00: return "Interrupt Controller";
        case 0x01: return "DMA Controller";
        case 0x02: return "Timer";
        case 0x03: return "RTC Controller";
        case 0x04: return "PCI Hot-Plug Controller";
        case 0x05: return "SD Host Controller";
        case 0x06: return "IOMMU";
        default: return "Other Base Peripheral Controller";
        }
    case 0x09: return "Input Device Controller";
    case 0x0A: return "Docking Station";
    case 0x0B: return "Processor";
    case 0x0C:
        switch (dev->id_subclass) {
        case 0x00: return "FireWire Controller";
        case 0x01: return "ACCESS Bus Controller";
        case 0x02: return "SSA Controller";
        case 0x03:
            switch (dev->id_prog_if) {
            case 0x00: return "USB Controller: UHCI";
            case 0x10: return "USB Controller: OHCI";
            case 0x20: return "USB Controller: EHCI";
            case 0x30: return "USB Controller: XHCI";
            default: return "Other USB Controller";
            }
        case 0x04: return "Fibre Channel Controller";
        case 0x05: return "SMBus Controller";
        case 0x06: return "InfiniBand Controller";
        case 0x07: return "IPMI Controller";
        case 0x08: return "SERCOS Controller";
        case 0x09: return "CAN Controller";
        default: return "Other Serial Bus Controller";
        }
    default: return NULL;
    }
}

int main(int argc, char **argv) {
    FILE *fp;
    DIR *dir;
    struct pci_device_config dev;
    struct dirent *ent;
    char path[256];

    dir = opendir("/sys/pci");
    if (!dir) {
        perror("/sys/pci");
        return -1;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') {
            continue;
        }

        snprintf(path, sizeof(path), "/sys/pci/%s/config", ent->d_name);
        fp = fopen(path, "r");
        if (fp == NULL) {
            perror(path);
            continue;
        }

        if (read_pci_device(&dev, fp) == 0) {
            const char *dev_type = pci_device_type(&dev);
            printf("%s: ", ent->d_name);
            if (dev_type) {
                printf("%s ", dev_type);
            } else {
                printf("Unknown Device (%02x:%02x:%02x) ", dev.id_class, dev.id_subclass, dev.id_prog_if);
            }
            printf("vendor=%04x, device=%04x\n", dev.id_vendor, dev.id_device);
        } else {
            printf("%s: Failed to read device\n", path);
        }

        fclose(fp);
    }

    closedir(dir);

    return 0;
}
