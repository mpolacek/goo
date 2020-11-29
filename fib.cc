// Fibonacci Number.
// This is O(n).

#include <utility>

int
fib (int n)
{
  if (n == 0)
    return 0;
  std::pair<int, int> v = {0, 1};
  for (int i = 1; i < n; ++i)
    v = {v.second, v.first + v.second};
  return v.second;
}

int
main ()
{
  __builtin_printf ("fib (%d) = %d\n", 6, fib (6));
  __builtin_printf ("fib (%d) = %d\n", 7, fib (7));
  __builtin_printf ("fib (%d) = %d\n", 8, fib (8));
  __builtin_printf ("fib (%d) = %d\n", 9, fib (9));
}
