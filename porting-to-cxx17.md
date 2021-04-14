# Migrating to C++17

In GCC 11, expected to be released in May 2021, the C++ default was [changed to C++17](https://gcc.gnu.org/gcc-11/changes.html) from C++14; in particular, the `-std=gnu++17` command-line option is now used by default.  C++17 brings a host of [new features](https://gcc.gnu.org/projects/cxx-status.html#cxx17), but it also deprecates, removes, or changes the semantics of certain constructs.  In the following article we take a look at some of the issues users may be facing when switching to GCC 11.  Remember that it is always possible to use the the previous C++ mode by using `-std=gnu++14`.  Moreover, this article only deals with the core language part; deprecated or removed features in the standard C++ library (such as `auto_ptr`) are not discussed here.  I encourage the reader to visit [this document](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0636r3.html) for a broader overview.  For more information regarding switching to using GCC 11, please see our [upstream document](https://gcc.gnu.org/gcc-11/porting_to.html).

### Trigraphs removed
In C++, a trigraph is a sequence of three characters starting with `??` that can express a single character.  For instance, `\` can be written as `??/`.  The reason for this is historical: C and C++ use special characters like `[` or `]` that are not defined in the ISO 646 character set.  The positions of these characters in the ISO table might be occupied by different characters in national ISO 646 characters sets, e.g. `¥` in place of `\`.  Trigraphs were meant to allow expressing such characters, but in practice, they are likely to only be used accidentally, and so were [removed](https://wg21.link/n4086) in C++17.  The removal allows you to play "cute" games like the following test.   Can you see why it works?  If for some reason you still need to use trigraphs in C++17 (indeed, there are code bases which [still use trigraphs](https://wg21.link/n2910)), GCC offers the `-trigraphs` command-line option.

```c++
bool cxx_with_trigraphs_p () {
  // Are we compiling in C++17??/
  return false;
  return true;
}
```

### `register` removed
The `register` keyword was deprecated in C++11 with [CWG 809](https://wg21.link/cwg809) because it had no practical effect.  C++17 cleaned this up further by [removing it](https://wg21.link/p0001r1) completely (though it remains reserved for future use).
Therefore, GCC 11 will warn (or issue an error with `-pedantic-errors`) by default in this test:

```c++
void f () {
  register int i = 42; // warning: ISO C++17 does not allow 'register' storage class specifier
  int register; // error
}
```

In C++14, you can instruct the compiler to warn using `-Wregister`, which ought to help with migrating code to C++17.  Note that GCC still accepts the GNU extension [explicit register variables](https://gcc.gnu.org/onlinedocs/gcc/Global-Register-Variables.html#Global-Register-Variables) without warning:

```c++
register int g asm ("ebx");
```

### Increment of `bool` removed
In C++, the `--` operator has never been supported for objects of type `bool`, but `++` on a `bool` had been only deprecated (starting in C++98).  In the spirit of C++17, post- and pre-increment is now forbidden, so GCC will give an error for code like the following.  Instead of `++`, simply use `b = true;` or `b |= true;`, depending on the specific case.

```c++
template<typename T>
void f() {
  bool b = false;
  b++; // error: use of an operand of type 'bool' in 'operator++' is forbidden in C++17
  T t{};
  t++; // error: use of an operand of type 'bool' in 'operator++' is forbidden in C++17
}

void g() {
  f<bool>();
}
```


### Exception specification changes

#### Exception specifications part of the type system
Since C++17, `noexcept` has become part of the type, meaning that these two functions:

```c++
int foo () noexcept;
int bar ();
```

had the same type in C++14, but have different types in C++17.  However, this doesn't mean that `noexcept` is part of a function's signature.  Therefore you cannot overload the following function `baz`:

```c++
int baz();
int baz() noexcept; // error: different exception specifier (even in C++11)
```

Another consequence is that in C++17, you can not convert a pointer to potentially-throwing function to pointer to non-throwing function:

```c++
void (*p)();
void (*q)() noexcept = p; // OK in C++14, error in C++17
```

This also affects template arguments.  The following program will not compile in C++17, because the compiler deduces two conflicting types for the template parameter `T`:

```c++
void g1 () noexcept;
void g2 ();
template<typename T> int foo (T, T);
void f() {
  foo (g1, g2); // error: void (*)() noexcept vs void (*)()
}
```

Interestingly, this change was first discussed over 20 years ago in [CWG 92](https://wg21.link/cwg92), and was finally adopted in [P0012R1](https://wg21.link/p0012).

#### Dynamic exception specifications removed
C++11 added `noexcept` exception-specification in [N3050](https://wg21.link/n3050), and at the same time *dynamic* exception specifications were deprecated in [N3051](https://wg21.link/n3051) (including `throw()`).  In C++17, dynamic exception specifications were removed altogether in [P0003R5](https://wg21.link/p0003).  The exception is that `throw()` continues to work in C++17, and is equivalent to `noexcept(true)`, although it has been removed in C++20.  Moreover, in C++17, the function `std::unexpected` was removed.  This function used to be called if a function decorated with `throw()` actually did throw an exception.  In C++17, `std::terminate` is called instead.  

C++17 code may not use dynamic exception specifications and should replace `throw()` with `noexcept`.

An example might help to clarify this:

```c++
void fn1() throw(int); // error in C++17, warning in C++14
void fn2() noexcept; // OK since C++11
void fn3() throw(); // deprecated but no warning in C++17
void fn4() throw() { throw; }
// In C++14, calls std::unexpected which calls std::unexpected_handler
// (which is std::terminate by default).
// In C++17, calls std::terminate directly.
```

### New template template parameter matching
The C++17 proposal [P0522R0](https://wg21.link/p0522), *Matching of template template-arguments excludes compatible templates* which fixed [DR 150](https://wg21.link/cwg150) was fixed in GCC 7, but the new behavior was off by default.  Since GCC 11 defaults to C++17, the new behavior is enabled by default.

In the old behavior, the template template-argument for a template template-parameter must be a template with a parameter list that exactly matches the corresponding parameters in the template parameter's parameter list.  The new behavior is less strict in that default template arguments of template template-arguments are considered for the matching.  This was problematic for instance for `std::deque` which is a template with a default template argument: 

```c++
template<typename T, typename Allocator = std::allocator<T>>
class deque;
```

The following code therefore only works with the new behavior (that is, `-fnew-ttp-matching` is in effect):

```c++
#include <deque>

template <template <typename> class>
void fn() {}
template void fn<std::deque>();
```

The workaround is to adjust the declaration in such a way that the parameter expects two type parameters:

```c++
#include <deque>

template <template <typename, typename> class>
void fn() {}
template void fn<std::deque>();
```

However, the new behavior can also cause code that worked in the old behavior to not compile anymore:

```c++
template <int N, int M = N> class A { };
template <int N, int M> void fn(A<N, M>) {}
template <int N, template <int> typename T> void fn(T<N>);

void
g ()
{
  A<3> a;
  fn (a); // ambiguous in C++17
}
```

The reason is that `A` is considered a valid argument for `T` in the new behavior. Therefore both function templates are valid candidates, and because neither is more specialized than the other,  the function call to `fn` has become ambiguous.

It's possible to revert to the old behavior even in C++17 mode by using `-fno-new-ttp-matching`.

### Static constexpr class members implicitly inline
The C++17 proposal to introduce inline variables ([P0386R2](https://wg21.link/p0386)) brought this change into [dcl.constexpr]: *A function or static data member declared with the constexpr or consteval specifier is implicitly an inline function or variable.*

As a consequence, the member `A::n` in the following example is a definition in C++17.  Without `#2`, the program wouldn't link when compiled in C++14 mode.  In C++17, `#2` may be removed because it's redundant.

```c++
struct A {
  static constexpr int n = 5; // #1, definition in C++17, declaration in C++14
};

constexpr int A::n; // #2, definition in C++14, deprecated redeclaration in C++17

auto g()
{
  return &A::n; // ODR-use of A::n -- needs a definition
}
```

### Evaluation order rules changes
C++17 [P0145R3](https://wg21.link/p0145) clarified the order of evaluation of various expressions.  As the proposal states, the following expressions are evaluated in such a way that `a` is evaluated before `b`:
   
1. `a.b`
2. `a->b`
3. `a->*b`
4. `a (b1, b2, b3)`
5. `b op= a`
6. `a[b]`
7. `a << b`
8. `a >> b`

This has resulted in some changes in the compiler, and certain code might behave differently in C++14 and C++17, as the following test illustrates.  It is possible to selectively adjust the compiler behavior using the `-fstrong-eval-order={all|some|none}` compile-time option, where `all` is the default in C++17 and `some` is the default in C++14.

```c++
int
fn ()
{
  int ar[4]{ };
  int i = 0;
  ar[i++] = i;
  return ar[0]; // returns 0 in C++17, 1 in C++14
}

int
fn2 ()
{
  int x = 2;
  return x << (x = 1, 2); // returns 8 in C++17, 4 in C++14
}

int
fn3 ()
{
  int x = 6;
  return x >> (x = 5, 1); // returns 3 in C++17, 2 in C++14
}
```

The aforementioned paper also defines the evaluation order in case some of the operands listed above are overloaded and are using the operator syntax: they follow the order prescribed for the built-in operator.

### Guaranteed copy elision

C++17 requires guaranteed copy elision, meaning that a copy/move constructor call will be elided completely under certain circumstances (like when the type of the initializer and target are the same), even when it has side effects.  That means that, theoretically, if something relied on a constructor being instantiated via e.g. copying a function parameter, it might now fail, as the constructor may not be instantiated in C++17.  Since G++ already performed copy/move elision as an optimization even in C++14 mode, this is unlikely to happen in practice.  However, the difference is that in C++17 the compiler will not perform access checking on the elided constructor, therefore code that didn't compile previously may compile now.  The following snippet shows this:

```c++
class A {
  int i;
public:
  A() : i{42} {}
private:
  A(const A &);
};

struct B {
  A a;
  B() : a(A()) {} // OK in C++17, error in C++14
};
```