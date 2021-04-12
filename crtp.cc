// CRTP, counting the number of instances.
// Uses extended friend declarations (C++11) instead of "protected:".

template<typename T>
class inst_counter {
  static int inst_count;
  friend T;
public:
  static int count() { return inst_count; }
};

struct A : inst_counter<A> {
  A() { ++inst_count; }
  A(const A&) { ++inst_count; }
  A(A&&) { ++inst_count; }
  ~A() { --inst_count; }
};

struct B : inst_counter<B> {
  B() { ++inst_count; }
  B(const B&) { ++inst_count; }
  B(B&&) { ++inst_count; }
  ~B() { --inst_count; }
};

B b;

template<typename T>
int inst_counter<T>::inst_count;

int
main ()
{
  A a1;
  A a2;
  A a3;
  {
    A a4;
    __builtin_printf ("A #1: %d\n", inst_counter<A>::count());
  }
  __builtin_printf ("A #2: %d\n", inst_counter<A>::count());
  __builtin_printf ("B: %d\n", inst_counter<B>::count());
}
