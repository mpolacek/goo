// Extended Euclidean algorithm.
// The extended Euclidean algorithm is an extension to the Euclidean algorithm,
// and computes, in addition to the greatest common divisor (gcd) of integers
// a and b, also the coefficients of Bézout's identity, which are integers x
// and y such that
//   xa + yb = gcd(a, b)
// From A. Stepanov - From Mathematics to Generic Programming.
// Use -std=c++20.

#include <concepts>
#include <cstdlib>
#include <utility>

#define assert(X) do { if (!(X)) std::abort (); } while(0)
using line_segment = unsigned int;

/* The ring of integers is an example of a Euclidean domain.  */
template<typename T>
concept euclidean_domain = std::integral<T>;

/* Find the first repeated doubling of B that exceeds the difference
   A - B.  */

static inline line_segment
largest_doubling (line_segment a, line_segment b)
{
  while (a - b >= b)
    b = b + b;
  return b;
}

static inline line_segment
half (line_segment n)
{
  return n / 2;
}

/* Returns a std::pair {quotient, remainder}.  Like std::div.  */

static std::pair<int, line_segment>
quotient_remainder (line_segment a, line_segment b)
{
  if (a < b)
    return {0, a};
  line_segment c = largest_doubling (a, b);
  int n = 1;
  a = a - c;
  while (c != b)
    {
      c = half (c);
      n = n + n;
      if (c <= a)
	{
	  a = a - c;
	  n++;
	}
    }
  return {n, a};
}

/* Returns a pair consisting of the value of X in the Bezout's identity,
   and the GCD of A and B.  */

template<euclidean_domain E>
std::pair<E, E> extended_gcd (E a, E b)
{
  E x0 = 1;
  E x1 = 0;

  while (b != E(0))
    {
      std::pair<E, E> qr = quotient_remainder (a, b);
      E x2 = x0 - qr.first * x1;
      /* Shift r and x.  */
      x0 = x1;
      x1 = x2;
      a = b;
      b = qr.second;
    }
  return {x0, a};
}

/* An application of the extended GCD: x is the modular multiplicative inverse
   of a modulo b, and y is the modular multiplicative inverse of b modulo a.
   NB: gcd (a, b) must be 1.  Which it will be if a and b are coprime.

   From Wiki: [...] This implies that the pair of Bézout's coefficients
   provided by the extended Euclidean algorithm is the minimal pair of
   Bézout coefficients, as being the unique pair satisfying both above
   inequalities.  */

template<euclidean_domain E>
E mult_inv_mod (E a, E b)
{
  auto i = extended_gcd (a, b).first;

  /* Also, for getting a result which is positive and lower than n, one may
     use the fact that the integer t provided by the algorithm satisfies
     |t| < n. That is, if t < 0, one must add n to it at the end.  */
  if (i < 0)
    i += b;

  return i;
}

int
main ()
{
  auto q = quotient_remainder (45, 6);
  __builtin_printf ("quotient_remainder: %d / %d = {%d, %d}\n", 45, 6, q.first, q.second);
  auto d = std::div (45, 6);
  __builtin_printf ("std::div: %d / %d = {%d, %d}\n", 45, 6, d.quot, d.rem);

  auto e = extended_gcd (196, 42);
  __builtin_printf ("extended_gcd (%d, %d) = {x = %d, gcd() = %d}\n", 196, 42,
		    e.first, e.second);
  auto e2 = extended_gcd (105, 252);
  __builtin_printf ("extended_gcd (%d, %d) = {x = %d, gcd() = %d}\n", 105, 252,
		    e2.first, e2.second);


  /* So if I want the modular multiplicative inverse of 4 mod 7:  */
  int p = 7;
  __builtin_printf ("mult inverse of %d mod %d = %d\n", 4, p,
		    mult_inv_mod (4, p));
  __builtin_printf ("mult inverse of %d mod %d = %d\n", 2, p,
		    mult_inv_mod (2, p));
  __builtin_printf ("mult inverse of %d mod %d = %d\n", 6, p,
		    mult_inv_mod (6, p));

  assert (mult_inv_mod (4, p) == 2);
  assert (mult_inv_mod (2, p) == 4);
  assert (mult_inv_mod (1, p) == 1);
  assert (mult_inv_mod (3, p) == 5);
  assert (mult_inv_mod (5, p) == 3);
  assert (mult_inv_mod (6, p) == 6);
}
