
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


void print(const char *str, unsigned long len)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    unsigned long written;

    WriteFile(h, str, len, &written, 0);
}
