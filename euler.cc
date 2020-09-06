// Euler’s Totient Function

#include <cstdlib>
#include <utility>

#define assert(X) do { if (!(X)) std::abort (); } while(0)

static constexpr std::pair<int, int> res[] = {
  { 1, 1 },
  { 2, 1 },
  { 3, 2 },
  { 4, 2 },
  { 5, 4 },
  { 6, 2 },
  { 7, 6 },
  { 8, 4 },
  { 9, 6 },
  { 10, 4 },
  { 11, 10 },
  { 12, 4 },
  { 13, 12 },
  { 14, 6 },
  { 15, 8 },
  { 16, 8 },
  { 17, 16 },
  { 18, 6 },
  { 19, 18 },
  { 20, 8 },
  { 21, 12 },
  { 22, 10 },
  { 23, 22 },
  { 24, 8 },
  { 36, 12 },
  { 48, 16 },
  { 60, 16 },
  { 72, 24 },
  { 84, 24 },
  { 96, 32 },
  { 100, 40 },
  { 108, 36 },
  { 120, 32 },
  { 121, 110 },
  { 122, 60 },
  { 123, 80 },
  { 124, 60 },
  { 125, 100 },
  { 140, 48 },
  { 141, 92 },
  { 142, 70 },
  { 143, 120 },
};

/* True iff N is prime.  */

static bool
prime_p (int n)
{
  if (n <= 3)
    return n > 1;
  else if ((n % 2) == 0 || (n % 3) == 0)
    return false;
  int i = 5;
  while (i * i <= n)
    {
      if ((n % i) == 0 || n % (i + 2) == 0)
	return false;
      i += 6;
    }
  return true;
}

/* https://en.wikipedia.org/wiki/Euler%27s_totient_function */

static int
phi (int n)
{
  if (prime_p (n))
    return n - 1;

  double r = 1.0;
  for (int p = 2; p < n; ++p)
    if (prime_p (p) && n % p == 0)
      r *= 1 - 1 / double(p);

  return n * r;
}

static void
do_test ()
{
  for (const auto &x : res)
    assert (phi (x.first) == x.second);
}

int
main ()
{
  do_test ();
}
