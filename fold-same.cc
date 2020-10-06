// Check if the types of all the arguments are the same using
// a fold expression.

#include <type_traits>

template<typename T1, typename... T>
constexpr bool same_types (T1, T...)
{
  return (std::is_same_v<T1, T> && ...);
}

static_assert (same_types (1, 2, 3));
static_assert (!same_types (1u, 1));
static_assert (same_types ("hi", "all"));
static_assert (!same_types (.1f, .1));
