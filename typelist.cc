// Typelist: a list of types.
// Use C++17.

#include <type_traits>

// The types are stashed in the parameter pack.
template<typename...>
struct typelist
{
};

template<typename>
struct front_t;

template<typename Head, typename... Tail>
struct front_t<typelist<Head, Tail...>>
{
  using type = Head;
};

// Extract the first element.
template<typename List>
using front = typename front_t<List>::type;

template<typename>
struct pop_front_t;

template<typename Head, typename... Tail>
struct pop_front_t<typelist<Head, Tail...>>
{
  using type = typelist<Tail...>;
};

// Remove the first element.
template<typename List>
using pop_front = typename pop_front_t<List>::type;

template<typename, typename>
struct push_front_t;

template<typename... Elts, typename New>
struct push_front_t<typelist<Elts...>, New>
{
  using type = typelist<New, Elts...>;
};

// Insert an element onto the front.
template<typename List, typename New>
using push_front = typename push_front_t<List, New>::type;

// Recursive case.
template<typename List, unsigned N>
struct get_nth_t : public get_nth_t<pop_front<List>, N - 1>
{
};

// Base case.
// Inherits from front_t, which provides ::type.
template<typename List>
struct get_nth_t<List, 0> : public front_t<List>
{
};

// Get the N-th element.
template<typename List, unsigned N>
using get_nth = typename get_nth_t<List, N>::type;

template<typename List, typename New>
struct push_back_t;

template<typename... Elts, typename New>
struct push_back_t<typelist<Elts...>, New>
{
  using type = typelist<Elts..., New>;
};

// Append an element.
template<typename List, typename New>
using push_back = typename push_back_t<List, New>::type;

template<typename List>
struct is_empty
{
  static constexpr bool value = false;
};

template<>
struct is_empty<typelist<>>
{
  static constexpr bool value = true;
};

// Is the typelist empty?
template<typename List>
constexpr bool is_empty_v = is_empty<List>::value;

template<typename List, bool Empty = is_empty_v<List>>
struct reverse_t;

template<typename List>
using reverse = typename reverse_t<List>::type;

// Recursive case.
// Split the list into the first element and the rest, then recursively
// reverse the list of the remaining elements, then append the first element
// to that reversed list.  E.g.,
//    typelist<short, int, long>
// -> short + typelist<int, long>
//    typelist<long, int>
//    typelist<long, int, short>
template<typename List>
struct reverse_t<List, false> : public push_back_t<reverse<pop_front<List>>, front<List>>
{
};

// Base case.
template<typename List>
struct reverse_t<List, true>
{
  using type = List;
};

template<typename List>
struct pop_back_t
{
  using type = reverse<pop_front<reverse<List>>>;
};

// Remove the last element.
template<typename List>
using pop_back = typename pop_back_t<List>::type;

int
main ()
{
  using l1 = typelist<int, bool, long, char>;
  static_assert (std::is_same_v<front<l1>, int>);
  using l2 = pop_front<l1>;
  static_assert (std::is_same_v<front<l2>, bool>);
  using l3 = push_front<l2, long long>;
  static_assert (std::is_same_v<front<l3>, long long>);

  //using e = front<typelist<>>; // error
  //using e = pop_front<typelist<>>; // error
  using x = push_front<typelist<>, short>;
  static_assert (std::is_same_v<front<x>, short>);
  using x2 = get_nth<l1, 1>;
  static_assert (std::is_same_v<x2, bool>);
  static_assert (is_empty_v<typelist<>>);
  static_assert (!is_empty_v<typelist<int>>);

  using l4 = push_back<push_back<typelist<>, bool>, char>;
  static_assert (std::is_same_v<front<l4>, bool>);

  using rev = reverse<l1>;
  static_assert (std::is_same_v<front<rev>, char>);

  using l5 = pop_back<l1>;
  static_assert (std::is_same_v<typelist<int, bool, long>, l5>);
}
