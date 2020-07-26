#include <iostream>

template<typename T, typename... Types>
void print (T t, Types... args)
{
  std::cout << t << '\n';
  // Without 'constexpr': error: no matching function for call to 'print()'
  if constexpr (sizeof...(args) > 0)
    print (args...);
}

int
main ()
{
  print<char, const char *, int>('a', "foo", 42);
}
