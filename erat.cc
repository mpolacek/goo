// C++20 implementation of the Sieve of Eratosthenes.

#include <array>
#include <concepts>
#include <iostream>
#include <iterator>

// Mark all the nonprimes for a factor.
template<std::random_access_iterator I, std::integral N>
void mark_sieve (I first, I last, N factor)
{
  *first = false;
  while (last - first > factor)
    {
      first += factor;
      *first = false;
    }
}

template<std::random_access_iterator I, std::integral N>
void sift (I first, N n)
{
  I last = first + n;
  std::fill (first, last, true);
  N i(0);
  N index_square(3);
  // Factor/value at index i.
  N factor(3);
  while (index_square < n)
    {
      // index_square = 2i^2 + 6i + 3
      if (first[i])
	mark_sieve (first + index_square, last, factor);
      ++i;
      // Index of the first value we want to mark.
      index_square += factor;
      factor += N(2);
      index_square += factor;
    }
}

int
main ()
{
  constexpr int sz = 30;
  std::array<bool, sz> a;
  sift (a.begin (), sz);

  for (int i = 0; i < sz; ++i)
    if (a[i])
      std::cout << 2 * i + 3 << "\n";
}
