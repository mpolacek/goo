// Miller-Rabin primality test.

#ifndef _GOO_MILLER_RABIN_H
#define _GOO_MILLER_RABIN_H 1

#if __cplusplus > 201703L && __cpp_concepts >= 201907L

#include <concepts>
#include <cstdlib>
#include <ctime>

/* Returns true iff N is odd.  */

template<std::integral I>
bool odd (I n)
{
  return n & 1;
}

/* Returns true iff N is even.  */

template<std::integral I>
static inline bool
even (I n)
{
  return !(n & 1);
}

/* Modular exponentiation using exponentiation by squaring.  B is the base,
   E the exponent, and M the modulus.  */

template<std::integral I>
I modular_pow (I b, I e, I m)
{
  if (m == 1)
    return 0;

  I r = 1;
  b %= m;
  while (e > 0)
    {
      if (odd (e))
	r = (r * b) % m;
      e >>= 1;
      b = (b * b) % m;
    }
  return r;
}

/* The Miller-Rabin test.  */

template<std::integral I>
static bool
miller_rabin (I q, I k, I n)
{
  /* Pick a random integer W in the range [2, n - 2].  */
  I w = 2 + std::rand () % (n - 4);

  /* w^q mod n */
  I x = modular_pow (w, q, n);

  if (x == 1 || x == n - 1)
    return true;

  for (I i = 1; i < k; ++i)
    {
      x = modular_pow (x, I(2), n);
      if (x == n - 1)
	return true;
      if (x == 1)
	return false;
    }

  /* N is composite.  */
  return false;
}

/* Return true if N is probably prime, and false if it definitely
   is not.  Uses the Miller-Rabin test.  */

template<std::integral I>
bool prime_p (I n)
{
  if (n == 2 || n == 3)
    return true;
  if (n < 2 || even (n))
    return false;

  /* Now n - 1 is even, we can write it as 2^k * q.  */
  I k = 0;
  I q = n - 1;
  while (even (q))
    q /= 2, ++k;

  /* Let's try it 4 times.  */
  for (int i = 0; i < 4; ++i)
    if (!miller_rabin (q, k, n))
      return false;

  return true;
}

#if 0
int
main ()
{
  std::srand (std::time (nullptr));

  for (int i = 1; i < 102; ++i)
    if (prime_p (i))
      __builtin_printf ("%d ", i);
  __builtin_printf ("\n");
}
#endif

#endif // C++2a

#endif // _GOO_MILLER_RABIN_H
