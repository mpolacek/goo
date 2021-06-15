// Dependent base.

template<typename>
struct B {
  int foo;
};

template<typename T>
struct D : B<T> { // dependent base
  // Make foo dependent by prepending "this->", otherwise
  // "not declared in this scope".
  void f() { int i = this->foo; }
};

template<>
struct B<bool> {
  enum { foo = 12 };
};

void
g (D<bool>& d)
{
  d.f();
}
