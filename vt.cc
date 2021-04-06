#include <iostream>

template<typename T, typename... Types>
void print (T t, Types... args)
{
  std::cout << t << '\n';
  // Without 'constexpr': error: no matching function for call to 'print()'
  if constexpr (sizeof...(args) > 0)
    print (args...);
}

template<typename... Ts>
void print_backwards (Ts... args)
{
  auto print_one = [](auto t) {
    std::cout << t << '\n';
    // Needs C++20.
    return std::type_identity<void>();
  };
  (print_one (args) = ...);
}

int
main ()
{
  print<char, const char *, int>('a', "foo", 42);
  print_backwards(1, "cat", 2, "raccoon", 3, "dog");
}
