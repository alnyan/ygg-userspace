#include <sys/utsname.h>
#include <stdio.h>

int main(int argc, char **argv) {
    struct utsname name;
    int res;

    if ((res = uname(&name)) != 0) {
        perror("uname()");
        return -1;
    }

    printf("%s %s %s %s\n",
        name.sysname,
        name.nodename,
        name.version,
        name.machine);

    return 0;
}
