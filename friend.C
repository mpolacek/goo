#include <iostream>

// 1) Define in the class.
template<typename T>
struct A {
  T t;
  constexpr A(T t_) : t{t_} { }
  void dump (std::ostream& s) const {
    s << t << std::endl;
  }
  // Temploid.
  friend std::ostream& operator<<(std::ostream& s, const A<T>& a)
  {
    a.dump (s);
    return s;
  }
};

// 2) Declare a new function template.
template<typename T>
struct B {
  T t;
  constexpr B(T t_) : t{t_} { }
  void dump (std::ostream& s) const {
    s << t << std::endl;
  }
  template<typename U>
  friend std::ostream& operator<<(std::ostream&, const B<U>&);
};

template<typename T>
std::ostream& operator<<(std::ostream& s, const B<T>& b)
{
  b.dump (s);
  return s;
}

// 3) Forward declare.
template<typename>
struct C;
template<typename T>
std::ostream& operator<< (std::ostream&, const C<T>&);

template<typename T>
struct C {
  T t;
  constexpr C(T t_) : t{t_} { }
  void dump (std::ostream& s) const {
    s << t << std::endl;
  }
  // Declare a specialization of the nonmember function template as friend.
  // NB: <T>, otherwise declares a non-template function.
  friend std::ostream& operator<< <T> (std::ostream&, const C<T>&);
};

template<typename T>
std::ostream& operator<<(std::ostream& s, const C<T>& c)
{
  c.dump (s);
  return s;
}

int
main ()
{
  A<int> a{42};
  std::cout << a;

  B<int> b{24};
  std::cout << b;

  C<int> c{11};
  std::cout << c;
}
