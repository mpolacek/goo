/* Use GCC 10+ and -menqcmd.  The .s file will contain
    enqcmds (%rcx), %rdx
    [...]
    enqcmd  (%rcx), %rdx
 */

#include <x86intrin.h>

unsigned int w;
unsigned int array[16];

int
test_enqcmds (void)
{
  return _enqcmds (&w, array);
}

int
test_enqcmd (void)
{
  return _enqcmd (&w, array);
}

int
main ()
{
  test_enqcmd ();
  test_enqcmds ();
}
