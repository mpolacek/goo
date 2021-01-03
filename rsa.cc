// A toy implementation of the RSA algorithm.
// Use -std=c++20.

#include "miller-rabin.h"
#include <cmath>
#include <iostream>
#include <numeric>
#include <utility>

#define assert(X) do { if (!(X)) std::abort (); } while(0)

using key_type = long int;
using key_pair = std::pair<key_type, key_type>;

/* The ring of integers is an example of a Euclidean domain.  */
template<typename T>
concept euclidean_domain = std::integral<T>;

/* Compute λ(n), where λ is Carmichael's totient function.

     λ(n) = lcm(p − 1, q − 1)
 */

template<std::integral I>
I carmichael_fn (I p, I q)
{
  return std::lcm (p - 1, q - 1);
}

/* Generate a prime around N.  */

template<std::integral I>
I generate_prime (I n)
{
  assert (odd (n));

  for (I i(n); i < 2 * n; i += 2)
    if (prime_p (i))
      return i;

  __builtin_unreachable ();
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
      auto qr = std::div (a, b);
      E x2 = x0 - qr.quot * x1;
      /* Shift r and x.  */
      x0 = x1;
      x1 = x2;
      a = b;
      b = qr.rem;
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

/* Generate a pair of RSA keys.

   Practical implementations use the Chinese remainder theorem to speed up
   the calculation using modulus of factors (mod pq using mod p and mod q).  */

[[nodiscard]] static std::pair<key_pair, key_pair>
generate_keys ()
{
  /* Generate two distinct prime numbers p and q.
     NB: These are very small, so this whole thing is extremely unsafe.  */
  key_type p = generate_prime (static_cast<key_type>(7919));
  key_type q = generate_prime (static_cast<key_type>(7331));
  assert (p != q);

  /* Compute n = pq.  */
  key_type n = p * q;

  /* Compute λ(n), where λ is Carmichael's totient function.  */
  key_type l = carmichael_fn (p, q);

  /* Choose an integer e such that 1 < e < λ(n) and gcd(e, λ(n)) = 1; that is,
     e and λ(n) are coprime.  */
  key_type e = 65537;
  assert (coprime_p (e, l));

  /* Compute the secret exponend d: de ≡ 1 (mod λ(n)).  */
  key_type d = mult_inv_mod (e, l);

  return {{n, e}, {n, d}};
}

/* Encrypt M using the public key KEY.  */

[[nodiscard]] static key_type
encrypt (key_type m, key_pair key)
{
  auto [n, e] = key;
  /* m = 0, 1, n - 1 means an unconcealed message.  */
  assert (m > 1 && m < n - 1);
  return modular_pow (m, e, n);
}

/* Decrypt C using the private key KEY.  */

[[nodiscard]] static key_type
decrypt (key_type c, key_pair key)
{
  auto [n, d] = key;
  return modular_pow (c, d, n);
}

/* Return true iff N is divisible by I.  */

template<std::integral I>
bool divides (I i, I n)
{
  return n % i == 0;
}

/* A (naive) attempt to factor a composite number N.  Just try to divide by
   every odd number starting from 3 up to sqrt (n).

  TODO: Try Pollard's rho heuristic.  */

[[gnu::const]] static key_type
factor_composite (key_type n)
{
  for (key_type i(3); i * i <= n; i += 2)
    if (divides (i, n))
      /* Bingo!  */
      return i;

  return key_type(0);
}

/* Given the ciphertext C and the public key KEY, attempt to
   decipher C.  */

static key_type
crack (key_type c, key_pair key)
{
  auto [n, e] = key;

  key_type p = factor_composite (n);
  if (p == key_type(0))
    /* Didn't work.  Oh well, we'll never know.  */
    return key_type(0);

  /* Get Q.  */
  key_type q = n / p;

  /* Now we can compute λ(n).  */
  key_type l = carmichael_fn (p, q);

  /* And figure out D.  */
  key_type d = mult_inv_mod (e, l);

  /* And decrypt the message.  */
  return decrypt (c, {n, d});
}

int
main ()
{
  auto [pub, prv] = generate_keys ();
  std::cout << "pub: (" << pub.first << ", " << pub.second << ")\n";
  std::cout << "prv: (" << prv.first << ", " << prv.second << ")\n";

  /* Message M should be padded; see PKCS#1 and OAEP .  */
  int m = 65;
  auto c = encrypt (m, pub);
  std::cout << "plaintext message " << m << " encrypted as " << c << "\n";
  auto m_ = decrypt (c, prv);
  assert (m == m_);
  std::cout << "decrypted " << c << " to " << m_ << "\n";

  /* What if we didn't know D?  We could attempt to factor N, compute
     λ(n), and figure out D.  */
  std::cout << "attempting to crack...\n";
  std::cout << "original message: " << crack (c, pub) << "\n";
}
