// Specialization of member function templates.

struct S {
  int val;

  template<typename T = int>
  T get () const
  {
    __builtin_printf ("primary\n");
    return val;
  }
};

// A full spec -> if in a header file, declare inline to avoid
// errors if the definition is included by different TUs.
template<>
inline bool S::get<bool> () const
{
  __builtin_printf ("<bool>\n");
  return !!val;
}

int
main ()
{
  S s{42};
  s.get();
  s.get<int>();
  s.get<bool>();
}
