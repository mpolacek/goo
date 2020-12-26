#include <algorithm>
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

// Using the Stein algorithm.

#define BinaryInteger typename

template<BinaryInteger N>
bool even (N n)
{
  return !(n & 1);
}

template<BinaryInteger N>
N stein_gcd (N m, N n)
{
  if (m < N(0))
    m = -m;
  if (n < N(0))
    n = -n;
  if (m == N(0))
    return n;
  if (n == N(0))
    return m;

  int d_m = 0;
  while (even (m))
    {
      m >>= 1;
      ++d_m;
    }

  int d_n = 0;
  while (even (n))
    {
      n >>= 1;
      ++d_n;
    }

  // odd (m) && odd (n)

  while (m != n)
    {
      if (n > m)
	std::swap (n, m);
      m -= n;
      do
	m >>= 1;
      while (even (m));
    }

  // m == n

  return m << std::min (d_m, d_n);
}

int
main ()
{
  if (gcm (45, 6) != 3)
    __builtin_abort ();

  if (gcd (45, 6) != 3)
    __builtin_abort ();

  if (stein_gcd (45, 6) != 3)
    __builtin_abort ();
  if (stein_gcd (196, 42) != 14)
    __builtin_abort ();
}
