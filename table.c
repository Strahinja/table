#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "defs.h"

int
version()
{
    printf("%s v%s\n", PROGRAMNAME, VERSION);
    return 0;
}

int
usage()
{
    printf("Usage: %s [-v|--version]\n", PROGRAMNAME);
    return 0;
}

int
error(int code, char *fmt, ...)
{
    char buf[BUFSIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    fprintf(stderr, "%s: %s", PROGRAMNAME, buf);
    return code;
}

int
main(int argc, char **argv)
{
    if (argc == 1)
    {
        return usage();
    }

    char *arg;
    int cmd = CMD_NONE;
    while (arg = *argv++) {
        if (*arg++ == '-')
        {
            char c = *arg++;
            if (c == '-')
            {
                if (!strcmp(arg, "version"))
                {
                    cmd = CMD_VERSION;
                }
                else
                {
                    error(1, "Invalid argument: --%s\n", arg);
                    return usage();
                }
            }
            else
            {
                switch (c)
                {
                    case 'v':
                        cmd = CMD_VERSION;
                        break;
                    default:
                        error(1, "Invalid argument: -%c\n", c);
                        return usage();
                }
            }
        }
        arg++;
    }

    if (cmd == CMD_VERSION)
    {
        return version();
    }

    error(2, "Invalid arguments\n");
    return usage();
}

