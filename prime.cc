// Primality testing.  Carmichael numbers.

#include <concepts>
#include <cstdlib>
#include <initializer_list>
#include <numeric>

#define assert(X) do { if (!(X)) std::abort (); } while(0)

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

/* True iff A and B are coprimes.  */

static bool
coprime_p (int a, int b)
{
  return std::gcd (a, b) == 1;
}

/* Return true iff N is divisible by I.  */

template<std::integral I>
bool divides (I i, I n)
{
  return n % i == 0;
}

/* Returns true iff N is even.  */

template<std::integral I>
bool even (I n)
{
  return !(n & 1);
}

/* Returns true iff N is odd.  */

template<std::integral I>
bool odd (I n)
{
  return n & 1;
}

/* Halve N.  */

template<std::integral I>
I half (I n)
{
  return n >> 1;
}

/* Return the smallest divisor of N.  N > 0.  */

template<std::integral I>
I smallest_divisor (I n)
{
  if (even (n))
    return 2;

  for (I i = 3; i * i <= n; i+= 2)
    if (divides (i, n))
      return i;
  return n;
}

/* True iff N is prime.  */

template<std::integral I>
bool is_prime (I n)
{
  return n > 1 && smallest_divisor (n) == n;
}

/* The above is slow -- it's exponential in the number of digits.  Here's
   a different approach.  */

template<std::integral I>
struct modulo_multiply {
  I modulus;
  modulo_multiply(const I& i) : modulus(i) {}

  I operator() (const I& n, const I& m) const {
    return (n * m) % modulus;
  }
};

template<std::integral I>
I identity_element (const modulo_multiply<I> &)
{
  return 1;
}

/* For now.  */
template<typename>
concept regular_type = true;

template<typename>
concept monoid_op = true;

template<typename>
concept semigroup_op = true;

template<regular_type A, std::integral N, semigroup_op Op>
A power_accumulate_semigroup (A r, A a, N n, Op op)
{
  if (n == 0)
    return r;
  assert (n > 0);
  while (true)
    {
      if (odd (n))
	{
	  r = op (r, a);
	  if (n == 1)
	    return r;
	}
      n = half (n);
      a = op (a, a);
    }
}

template<regular_type A, std::integral N, semigroup_op Op>
A power_semigroup (A a, N n, Op op)
{
  assert (n > 0);
  while (!odd (n))
    {
      a = op (a, a);
      n = half (n);
    }
  if (n == 1)
    return a;
  return power_accumulate_semigroup (a, op (a, a), half (n - 1), op);
}

template<regular_type A, std::integral N, monoid_op Op>
A power_monoid (A a, N n, Op op)
{
  if (n == 0)
    return identity_element (op);
  assert (n > 0);
  return power_semigroup (a, n, op);
}

/* We want to compute a multiplicative inverse modulo prime p.
   Fermat: the inverse of a, where 0 < a < p is a^{p - 2}.
   (Inverse WRT p means having a remainder of 1 after dividing by p.

   P is prime, A > 0.  */

template<std::integral I>
I multiplicative_inverse_fermat (I a, I p)
{
  return power_monoid (a, p - 2, modulo_multiply<I>(p));
}

/* Fermat's Little Theorem:

     If p is prime, then a^{p - 1} - 1 is divisible by p for any 0 < a < p.

   which means:

     If p is prime, then a^{p - 1} = 1 mod p for any 0 < a < p.

  We want to know if N is prime.  Take an arbitrary number a smaller than n,
  raise it to the n - 1 power and check if the result is 1.  If not, n is
  *not* prime.  If it is 1, there's a chance that n is prime.  Try random a.  */

template<std::integral I>
bool fermat_test (I n, I a)
{
  assert (a > 0 && a < n);
  I r = power_semigroup (a, n - 1, modulo_multiply<I>(n));
  return r == 1;
}

/* Try out the above.  */

template<std::integral I>
bool try_it (I n)
{
  if (!fermat_test (n, n - 1))
    return false;
  if (!fermat_test (n, n / 2))
    return false;
  if (!fermat_test (n, n / 3))
    return false;
  if (!fermat_test (n, n / 4))
    return false;

  /* Assume N is prime.  */
  return true;
}

/* The above will be fooled by Carmichael numbers!  If we try a's coprime
   to N.  Try for 172081, a Carmichael number, with prime factorization
   7 * 13 * 31 * 61.  */

static void
try_carm ()
{
  constexpr int n = 172081;
  // NB: larger numbers would cause an overflow.
  for (auto i : { 9, 81, 123 })
    {
      if (coprime_p (n, i) && fermat_test (n, i))
	/* Fooled as expected.  */;
      else
	assert (!"huh?");
    }
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

/* True iff N is a Carmichael number.

   A composite number n > 1 is a Carmichael number iff

     for each b > 1, coprime(b, n) => b^{n - 1} = 1 mod n

   Here, b^{n - 1} may be an extremely large number, so we have to use
   modular exponentiation.  */

template<std::integral I>
bool is_carmichael (I n)
{
  if (n <= 1 || is_prime (n))
    return false;
  for (I b = 2; b < n; ++b)
    if (coprime_p (b, n))
      {
	I r = modular_pow (b, n - 1, n);
	if ((r % n) != 1)
	  return false;
      }
  return true;
}

/* Find the first N Carmichael numbers.  */

static void
find_first_n_carmichaels (int n)
{
  __builtin_printf ("First %d Carmichael numbers:", n);
  for (int i = 2; i < std::numeric_limits<int>::max() && n > 0; ++i)
    if (is_carmichael (i))
      {
	__builtin_printf (" %d", i);
	--n;
      }
  __builtin_printf ("\n");
}

int
main ()
{
  assert (prime_p (7753));
  assert (!prime_p (7791));
  assert (is_prime (7753));
  assert (!is_prime (7791));
  assert (try_it (11));
  assert (!try_it (12));
  assert (try_it (7753));
  assert (!try_it (7791));

  /* Let's try a Carmichael number (which is not prime).  */
  assert (!prime_p (172081));
  assert (!is_prime (172081));
  try_carm ();

  assert (modular_pow (5, 3, 13) == 8);
  assert (modular_pow (4, 13, 497) == 445);

  assert (is_carmichael (172081L));
  assert (!is_carmichael (7753));
  assert (!is_carmichael (7741));
  assert (!is_carmichael (560));
  assert (is_carmichael (561));
  assert (is_carmichael (1105));

  find_first_n_carmichaels (7);
}
