// Collatz conjecture.
// Unsolved problem in mathematics:
// Does the Collatz sequence eventually reach 1 for all positive integer
// initial values?

static void
collatz (unsigned long n)
{
  __builtin_printf ("n == %lu:", n);
  while (n != 1)
    {
      if ((n & 1) == 0)
	n /= 2;
      else
	n = 3 * n + 1;
      __builtin_printf (" -> %lu", n);
    }
  __builtin_printf ("\n");
}

int
main ()
{
  collatz (7ul);
  collatz (12ul);
  collatz (27ul);
  collatz (871ul);
}
