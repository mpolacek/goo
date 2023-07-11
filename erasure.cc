class Doer {
private:
  const void *ob;
  int (*fn)(const void *);
public:
  template <typename T>
  constexpr Doer(const T &t)
    : ob{&t},
      fn{[](const void *p) { return static_cast<const T *>(p)->doit(); }}
  {}
  constexpr int operator()() const { return fn(ob); }
};
struct Thing { constexpr int doit() const { return 42; }; };
static_assert (Doer(Thing())() == 42);
