#include <iostream>
#include <tuple>
#include <utility>

template<typename T, T... Ints>
void debug (std::integer_sequence<T, Ints...> seq)
{
  std::cout << "[size: " << seq.size() << "] < ";
  ((std::cout << Ints << ' '), ...);
  std::cout << ">\n";
}

int
main ()
{
  debug (std::integer_sequence<int, 2, 3, 5, 7, 11>{});
  debug (std::index_sequence<2, 3, 5, 7, 11>{});
  debug (std::make_integer_sequence<int, 6>{});
  debug (std::make_index_sequence<7>{});
  debug (std::index_sequence_for<float, std::iostream, char>{});
}
