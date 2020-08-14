#include <cstddef>
#include <utility>

// Recursive Remainder Lemma:
// If r = segment_remainder (a, 2b), then
//
// segment_remainder (a, b) =
// 1) r  if  r <= b
// 2) r - b  if  r > b

size_t
rem (size_t a, size_t b)
{
  if (a <= b)
    return a;
  if (a - b <= b)
    return a - b;
  a = rem (a, 2 * b);
  if (a <= b)
    return a;
  return a - b;
}

size_t
gcm (size_t a, size_t b)
{
  while (a != b)
    {
      a = rem (a, b);
      std::swap (a, b);
    }
  return a;
}

// If we can use %.
int
gcd (int a, int b)
{
  while (b != 0)
    {
      a = a % b;
      std::swap (a, b);
    }
  return a;
}

int
main ()
{
  if (gcm (45, 6) != 3)
    __builtin_abort ();

  if (gcd (45, 6) != 3)
    __builtin_abort ();
}
