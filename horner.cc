// Horner’s Method for Polynomial Evaluation.
// <https://en.wikipedia.org/wiki/Horner's_method>
// Can be used to convert a number from a non-DEC base to DEC.

#if __cplusplus > 201703L

#include <concepts>
#include <span>

template<std::integral I, typename T, std::size_t N> [[nodiscard]]
constexpr auto horner (I x, std::span<T, N> s)
{
  I r = s.front();
  for (const auto& e : s.subspan(1))
    r = r * x + e;
  return r;
}

#else

using size_t = decltype(sizeof(0));

template <typename T, size_t N>
constexpr size_t array_size(T (&)[N]) {
  return N;
}

constexpr long int
horner (long int x, const long int a[], size_t sz)
{
  long int r = a[0];
  for (size_t i = 1; i < sz; ++i)
    r = r * x + a[i];
  return r;
}
#endif

int
main ()
{
  constexpr long int a[]{ 14, 4, 9, 0, 13, 15 };
  constexpr long int x = 16;
#if __cplusplus > 201703L
  constexpr auto r = horner (x, std::span{a});
#else
  constexpr auto r = horner (x, a, array_size (a));
#endif
  static_assert (r == 14979295L);
}
