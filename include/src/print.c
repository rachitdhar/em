
//
// print.c
//

/*

This is a C implementation of the print
function, that makes use of the Windows
kernel32.lib API functions to write to
stdout.

TODO:
    Eventually I hope to be able to
    implement this print function in
    Em itself. At the moment the language
    does not have pointers (and I also
    would have to think about stuff like
    __stdcall and whether I need to do
    something for that), but it would be
    really useful to get started with some
    print functionality. So until then this
    C libarary would do.

NOTE: __stdcall before a function is needed
to specify that the arguments are removed by
the callee (whereas by default - i.e. __cdecl -
the args are removed from the stack by the
caller).

*/

#include <stdarg.h>

#define STD_OUTPUT_HANDLE -11

typedef void* HANDLE;
typedef unsigned long DWORD;
typedef int BOOL;


HANDLE __stdcall GetStdHandle(DWORD nStdHandle);

BOOL __stdcall WriteFile(
    HANDLE hFile,
    const void *buffer,
    DWORD bytesToWrite,
    DWORD *bytesWritten,
    void *overlapped
);


static HANDLE stdout_handle = 0;

// to avoid repeated calls to GetStdHandle
static HANDLE get_stdout()
{
    if (!stdout_handle) {
        stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    }
    return stdout_handle;
}

// to write a single char to stdout
void putchar(const char c)
{
    DWORD written;
    WriteFile(get_stdout(), &c, 1, &written, 0);
}

void print_int(int value) {
    if (value == 0) {
        putchar('0');
        return;
    }

    if (value < 0) {
        putchar('-');
        value = -value;
    }

    char buffer[20];
    int i = 0;

    while (value > 0) {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }

    while (i--) putchar(buffer[i]);
}

void print_str(const char *str) {
    while (*str) putchar(*str++);
}

// implementation of a simple printf function
// that handles the format specifiers and prints
// the data from the args provided accordingly.
void print(const char *format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == '%') {
            format++;

            switch (*format) {
                case 'd': {
                    int val = va_arg(args, int);
                    print_int(val);
                    break;
                }
                case 's': {
                    const char *str = va_arg(args, const char *);
                    print_str(str);
                    break;
                }
                case 'c': {
                    int ch = va_arg(args, int);
                    putchar(ch);
                    break;
                }
                case '%': {
                    putchar('%');
                    break;
                }
                default: {
                    // unknown specifier, print as-is
                    putchar('%');
                    putchar(*format);
                    break;
                }
            }
        } else {
            putchar(*format);
        }
        format++;
    }

    va_end(args);
}
