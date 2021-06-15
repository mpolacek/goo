// Counterintuitive behavior with a nondependent base.

template<typename>
struct B {
  using T = int;
};

template<typename T>
struct D : B<double> {
  // When T, an unqualified name, is looked up here in the derivation,
  // the nondependent bases are searched first before the template parms.
  // T == int
  T t = 42;
};

struct X { };
D<X> d;
