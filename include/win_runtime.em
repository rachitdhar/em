/*

Em Runtime for Windows
*******************************************

This is the actual entry point of any EM
program. The main() function is called from
over here, and the ExitProcess ends the
program towards the end.

This runtime invokes the Windows OS APIs
to perform all these functions.

*/

u32 main();
void ExitProcess(u32 result);


void _start()
{
    u32 result = main();
    ExitProcess(result);
}
