# New C++ features in GCC 12

The GNU Compiler Collection (GCC) 12.1 is expected to be released in April 2022.  Like every major GCC release, this version will bring many [additions, improvements, bug fixes, and new features](https://gcc.gnu.org/gcc-12/changes.html).  GCC 12 is already the system compiler in [Fedora 36](https://fedoraproject.org/wiki/Changes/GNUToolchainF36).  GCC 12 will also be available on [Red Hat Enterprise Linux](https://developers.redhat.com/products/rhel/overview) in the Red Hat Developer Toolset (RHEL 7), or the Red Hat GCC Toolset (RHEL 8 and 9).

As the similar blog post I wrote about [GCC 10](https://developers.redhat.com/blog/2020/09/24/new-c-features-in-gcc-10), this article only describes new features in the C++ front end.

We implemented several C++23 proposals in GCC 12.  The default dialect in GCC 12 is `-std=gnu++17`; to enable C++23 features, use the `-std=c++23` or `-std=gnu++23` command-line option. (The latter option allows GNU extensions.) 

Note that C++20 and C++23 are still experimental in GCC 12.

## C++23 features

### `if consteval`

C++17 introduced the constexpr if statement.  The condition in `if constexpr` must be a constant expression (it is *manifestly constant evaluated*).  If the condition evaluates to true, the "else" branch, if present, is discarded (which means, for instance, that the "else" branch will not be instantiated, which is a different behavior from an ordinary `if`).  If the condition evaluates to false, the "then" branch is discarded.

C++20 introduced the `consteval` keyword.  A (possibly member) function or a constructor marked as `consteval` is an *immediate function*.  Immediate functions must produce a constant when they are called (unless the call to an immediate function takes place in another immediate function); if they don't, the compiler will produce an error.  The compiler will not emit any actual code for such functions.

In contrast, a `constexpr` function may or may not be evaluated at compile-time -- depending on the context.  To that end, C++20 introduced a new library function `std::is_constant_evaluated`, which returns true if the current context is "evaluated at compile time":

```c++
#include <type_traits>

int slow (int);

constexpr int fn (int n)
{
  if (std::is_constant_evaluated ())
    return n << 1; // #1
  else
    return slow (n); // #2
}

constexpr int i = fn (10); // does #1
int n = 10;
int i2 = fn (n); // calls slow #2
```

However, the language rules do not allow the user to replace `n << 1` in the test case above with a call to a `consteval` function:

```c++
#include <type_traits>

int slow (int);
consteval int fast (int n) { return n << 1; }

constexpr int fn (int n)
{
  if (std::is_constant_evaluated ())
    return fast (n); // 'n' is not a constant expression
  else
    return slow (n);
}
constexpr int i = fn (10);
```

To fix this problem, [P1938R3](https://wg21.link/p1938) introduced `if consteval`, which GCC 12 implements.  `if consteval` allows the user to invoke immediate functions as shown below:

```c++
#include <type_traits>

int slow (int);
consteval int fast (int n) { return n << 1; }

constexpr int fn (int n)
{
  if consteval {
    return fast (n); // OK
  } else {
    return slow (n);
  }
}

constexpr int i = fn (10);
```

Note that it is valid to have `if consteval` in an ordinary, non-`constexpr` function.  Also note that `if consteval` requires `{ }`, unlike the ordinary if statement.

There is a problem with the interaction between `if constexpr` and `std::is_constant_evaluated`, but fortunately the compiler can  detect that problem.  See the *Extended `std::is_constant_evaluated` in `if` warning* paragraph below.

### `auto(x)`

GCC 12 implements proposal [P0849](https://wg21.link/p0849), which allows `auto` in a *function-style cast*, the result of which is a [*prvalue*](https://en.cppreference.com/w/cpp/language/value_category):

```c++
struct A {};
void f(A&);  // #1
void f(A&&); // #2
A& g();

void
h()
{
  f(g()); // calls #1
  f(auto(g())); // calls #2 with a temporary object
}
```

Note that both `auto(x)` and `auto{x}` are accepted; however, `decltype(auto)(x)` remains invalid.

### Non-literal variables in constexpr functions

GCC 12 implements C++23 [P2242R3](https://wg21.link/p2242), which allows non-literal variables, gotos, and labels in `constexpr` functions so long as they are not constant evaluated.  This is useful for code like (taken from the proposal above):

```c++
#include <type_traits>

template<typename T> constexpr bool f() {
  if (std::is_constant_evaluated()) {
    return true;
  } else {
    T t; // OK when T=nonliteral in C++23
    return true;
  }
}
struct nonliteral { nonliteral(); };
static_assert(f<nonliteral>());
```

The example above does not compile in C++20, but compiles in C++23, because the else branch is not evaluated.  Furthermore, the following example also only compiles in C++23:

```c++
constexpr int
foo (int i)
{
  if (i == 0)
    return 42;
  static int a;
  thread_local int t;
  goto label;
label:
  return 0;
}
```

### Multidimensional subscript operator

GCC 12 supports C++23 [P2128R6](https://wg21.link/p2128) -- multidimensional subscript operator.  A comma expression within a subscripting expression was deprecated in C++20 via [P1161R3](https://wg21.link/p1161), and in C++23 the comma in `[ ]` has changed meaning.

C++ uses the `operator[]` member function to access the elements of an array, or array-like types such as `std::array`, `std::span`, `std::vector`, or `std::string`.  However, this operator did not accept multiple arguments in C++20, so accessing the elements of multidimensional arrays was implemented using workarounds such as function call operators `arr(x, y, z)` and similar.  These workarounds have a number of drawbacks, so to alleviate the issues when using these workarounds, C++23 allows that `operator[]` takes zero or more arguments.
As a consequence, this test case is accepted with `-std=c++23`:

```c++
template <typename... T>
struct W {
  constexpr auto operator[](T&&...);
};

W<> w1;
W<int> w2;
W<int, int> w3;
```

Or, perhaps a clearer example (with a very naive implementation):

```c++
struct S {
  int a[64];
  constexpr S () : a {} {};
  constexpr S (int x, int y, int z) : a {x, y, z} {};
  constexpr int &operator[] () { return a[0]; }
  constexpr int &operator[] (int x) { return a[x]; }
  constexpr int &operator[] (int x, long y) { return a[x + y * 8]; }
};

void g ()
{
  S s;
  s[] = 42;
  s[5] = 36;
  s[3, 4] = 72;
}
```

As an extension, GCC still supports the old behavior when an overloaded subscript operator is not found, though it will issue a warning:

```c++
void f(int a[], int b, int c)
{
  a[b,c]; // deprecated in C++20, invalid but accepted with a warning in C++23
  a[(b,c)]; // OK in both C++20 and C++23
 }
```

Note that currently `operator[]` does not support default arguments.  It appears, though, that default arguments will be allowed: see [CWG 2507](https://wg21.link/cwg2507).  If/when the proposed adjustment is accepted, the following example will be allowed:

```c++
struct X {
  int a[64];
  constexpr int& operator[](int i = 1) { return a[i]; }
};
```

### `elifdef` and `elifndef`

In C and C++, the `#ifdef` and `#ifndef` preprocessing directives are "syntactic sugar" for `#if defined(something)` and `#if !defined(something)`.  Surprisingly, the else variants of these directives did not have the same shorthands.  To amend this omission, both C and C++ accepted proposals [N2645](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n2645.pdf) and [P2334R1](https://wg21.link/p2334), respectively.  GCC 12 implements both proposals and therefore the following example compiles correctly:

```c
#ifdef __STDC__
/* ... */
#elifndef __cplusplus
#warning "not ISO C"
#else
/* ... */
#endif
```

Please note that the code example above will compile without errors in C++20 and earlier only when GNU extensions are enabled.  That is, `-std=c++20` will cause a compile error, but `-std=gnu++20` will cause only a pedantic warning if `-Wpedantic` was turned on.

### Extended *init-statement*

GCC 12 implements C++23 [P2360R0](https://wg21.link/p2360r0), which merely extends *init-statement* (used in `if`/`for`/`switch` statements) to contain *alias-declaration*.  In practice, that means that the following code is accepted:

```c++
for (using T = int; T e : v)
  {
    // use e
  }
```

## Other changes

### Dependent operator lookup changes

GCC 12 corrected unqualified lookup for a dependent operator expression at template definition time instead of at instantiation time, as was already the case for dependent call expressions.  Consider the following test case demonstrating this change:

```c++     
#include <iostream>
         
namespace N {
  struct A { };
}
         
void operator+(N::A, double) {
  std::cout << "#1 ";
}
         
template<class T>
void f(T t) {
  operator+(t, 0);
  t + 0;
}
         
// Since it's not visible from the template definition, this later-declared
// operator overload should not be considered when instantiating f<N::A>(N::A),
// for either the call or operator expression.
void operator+(N::A, int) {
  std::cout << "#2 ";
}
         
int main() {
  N::A a;
  f(a);
  std::cout << std::endl;
}
```

This program will print `#1 #2` when compiled with GCC 11 or older, but GCC 12 correctly prints `#1 #1`.  That is, previously only the call expression resolved to the `#1` overload, but with GCC 12 the operator expression does too.

### `auto` specifier for pointers and references to arrays

GCC 12 also supports [DR2397](https://wg21.link/cwg2397), `auto` specifier for pointers and references to arrays.  This change removes the restriction that the array element type may not be a *placeholder type*.  This allows code like
    
```c++
int a[3];
auto (*p)[3] = &a;
auto (&r)[3] = a;
```    

 However, note that
 
```c++
auto (&&r)[2] = { 1, 2 };
auto arr[2] = { 1, 2 };
```    

still doesn't work (although one day it might) and neither does
   
```c++ 
int arr[5];
auto x[5] = arr;
```    

given that `auto` deduction is performed in terms of function template argument deduction, so the array decays to a pointer.

### Folding of trivial functions

A well-formed call to `std::move`/`std::forward` is equivalent to a cast, but the former being a function call means the compiler generates debug info, which persists even after the call gets inlined, for an operation that's never interesting to debug.  Therefore, GCC 12 elides calls to certain trivial inline functions such as `std::move`, `std::forward`, `std::addressof`, and `std::as_const` into simple casts as part of the front end's general expression folding routine.  As a result, the debug info produced by GCC may now be up to 10% smaller while improving GCC's compile time and memory usage.  This new behavior is controlled by a new flag called `-ffold-simple-inlines`.

### DR2374, Overly permissive specification of `enum` direct-list-initialization

GCC 12 implements defect report [DR2374](https://wg21.link/cwg2374), which forbids, for instance,
*direct-list-initialization* of a scoped enumeration from a different
scoped enumeration:

```c++
enum class Orange;
enum class Apple;
Orange o;
Apple a{o}; // error with GCC 12
```

### DR 1315, Restrictions on non-type template arguments in partial specializations 

Before this defect report was accepted, an overly strict restriction prevented certain uses of template parameters as template arguments.  This restriction has been rectified, and GCC implements it.  Therefore the following plausible use of a template parameter as a template argument compiles OK:

```c++
template <int I, int J> struct A {};
template <int I> struct A<I, I*2> {}; // OK with GCC 12
```

### DR 1227, Substitute into function parameters in lexical order

C++ template argument deduction underwent some changes to clarify that the substitution proceeds in lexical order, that is, in left-to-right order.  The following code demonstrates what effect this might have:

```c++
template <typename T>
struct A { using type = typename T::type; };

template <typename T> void g(T, typename A<T>::type);
template <typename T> long g(...);

long y = g<void>(0, 0); // OK in GCC 12, error in GCC 11

template <class T> void h(typename A<T>::type, T);
template <class T> long h(...);

long z = h<void>(0, 0); // error in GCC 12, OK in GCC 11
```

GCC 12 substitutes the arguments in left-to-right order, and checks if a substituted type is erroneous before substituting into the rest of the arguments.  Thus, for `g<void>(0, 0)` the compiler tries to substitute `void` into `g(T, typename A<T>::type)` and sees that the first substitution results in an invalid parameter type `void`, which is a SFINAE failure, so the first overload is discarded and the `g(...)` one is chosen instead.  However, for `h<void>(0, 0)` the compiler first substitutes `void` into the `typename A<T>::type` parameter.  This will produce a hard error, since instantiating `A<void>` is not an immediate context.

GCC 11 and earlier performed the substitution in right-to-left order, so the situation was reversed: `g<void>(0, 0)` resulted in a compile error whereas `h<void>(0, 0)` compiled fine.

### Stricter checking of attributes on friend declarations

If a friend declaration has an attribute, that declaration must be a definition, but before GCC 12 this wasn't checked.
Moreover, a C++11 attribute cannot appear in the middle of the *decl-specifier-seq*:

```c++
template<typename T>
struct S {
  [[deprecated]] friend T; // warning: attribute ignored
  [[deprecated]] friend void f(); // warning: attribute ignored
  friend [[deprecated]] int f2(); // error
};
S<int> s;
```

### Deduction guides can be declared at class scope

Due to a bug, deduction guides could not be declared at class scope in GCC 11 and earlier.  This has been [fixed in GCC 12](https://gcc.gnu.org/PR79501), so the following test case compiles correctly:

```c++
struct X {
  template<typename T> struct A {};
  A() -> A<int>;
};
```

Class-scope non-template deduction guides are now supported as well in GCC 12.

### Ordered comparison of null pointers is now rejected

Relational comparisons between null pointer constants and pointers are
ill-formed and this is diagnosed in GCC 12:

```c++
decltype(nullptr) foo ();
auto cmp = foo () > 0; // error: ordered comparison of pointer with integer zero
```


Note: You can find the overall defect resolution status on the [C++ Defect Report Support in GCC](https://gcc.gnu.org/projects/cxx-dr-status.html) page.

## New and improved warnings

### `-Wuninitialized` extended

The `-Wuninitialized` warning has been extended to warn about using uninitialized variables in member initializer lists.  Therefore the front end can detect bugs like:

```c++
struct A {
  int a;
  int b;
  A() : b(1), a(b) { }
};
```

where the field `b` is used uninitialized because the order of member initializers in the member initializer list is irrelevant; what matters is the order of declarations in the class definition.  (A related warning, `-Wreorder`, can be used to warn when the order of member initializers does not match the declaration order.)

The warning does not warn about more complex initializers.  And it also does not warn when the address of an object is used:

```c++
struct B {
  int &r;
  int *p;
  int a;
  B() : r(a), p(&a), a(1) { } // no warning
};
```

As an aside, the request to enhance the warning came about 17 years ago.  Apparently, sometimes things take their time.

### `-Wbidi-chars` added

The `-Wbidi-chars` warning warns about potentially misleading UTF-8 bidirectional control characters, which can change left-to-right writing direction into right-to-left (and vice versa).  As a consequence, the BiDi characters might cause confusion for the programmer because seemingly commented-out code might actually be used, or vice versa.  This warning is supposed to mitigate [CVE-2021-42574](https://nvd.nist.gov/vuln/detail/CVE-2021-42574) aka [Trojan Source](https://trojansource.codes/).

For more information, please refer to a [blog post](https://developers.redhat.com/articles/2022/01/12/prevent-trojan-source-attacks-gcc-12) by David Malcolm.

### `-Warray-compare` added

The new `-Warray-compare` warning warns about comparisons between two operands of array type, which was deprecated in C++20.  For example:

```c++
int arr1[5];
int arr2[5];
bool same = arr1 == arr2; // warning: comparison between two arrays
```

### `-Wattributes` extended

The `-Wattributes` warning has been extended and users can now use `-Wno-attributes=ns::attr` or `-Wno-attributes=ns::` to suppress warnings about unknown scoped attributes (in C++11 and C2X). Similarly, `#pragma GCC diagnostic ignored_attributes "ns::attr"` can be used to achieve the same effect.  The new functionality is meant to help with vendor-specific attributes, where a warning is not desirable, while still detecting typos.  Consider:

```c++
[[deprecate]] void g(); // warning: should be deprecated
[[company::attr]] void f(); // no warning
```

which, when compiled with `-Wno-attributes=company::`, only issues one warning.

### New warning options for C++ language mismatches

GCC 12 gained new warning flags, enabled by default: `-Wc++11-extensions`, `-Wc++14-extensions`, `-Wc++17-extensions`, `-Wc++20-extensions`, and `-Wc++23-extensions`.  These flags can be used to control existing pedantic warnings about occurrences of new C++ constructs in code using an older C++ standard dialect.  For instance, users are now able to suppress warnings when using variadic templates in C++98 code by applying the new `-Wno-c++11-extensions` option.

### Extended `std::is_constant_evaluated` in `if` warning

Since the condition in `if constexpr` is *manifestly constant evaluated*, `if constexpr (std::is_constant_evaluated())` is always true.  GCC 10 introduced a warning to detect this bug, and GCC 12 extended the warning to detect more dubious cases.  For instance:

```c++
#include <type_traits>

int
foo ()
{
  if (std::is_constant_evaluated ()) // warning: always evaluates to false in a non-constexpr function
    return 1;
  return 0;
}

consteval int
baz ()
{
  if (std::is_constant_evaluated ()) // warning: always evaluates to true in a consteval function
    return 1;
  return 0;
}
```

### `-Wmissing-requires` added

The `-Wmissing-requires` warning warns about a missing `requires`.  Consider

```c++
template <typename T> concept Foo = __is_same(T, int);

template<typename Seq>
concept Sequence = requires (Seq s) {
  /* requires */ Foo<Seq>;
};
```

where the user presumably meant to invoke the `Foo` concept (nested requirement), which needs to be prefixed by the keyword `requires`.  In this test, `Foo<Seq>` is a *concept-id* which will make `Sequence` true if `Foo<Seq>` is a valid expression, which it is for all `Seq`.

### `-Waddress` enhanced

The `-Waddress` warning has been extended.  It now warns about, for instance, comparing the address of a nonstatic member function to the null pointer value:

```c++
struct S {
  void f();
};

int g()
{
  if (&S::f == nullptr) // warning: the address &S::f will never be NULL
    return -1;
  return 0;
}
```

## Acknowledgments

As usual, I'd like to thank my coworkers at Red Hat who made the GNU C++ compiler so much better, notably Jason Merrill, Jakub Jelinek, Patrick Palka, and Jonathan Wakely.

## Conclusion
In GCC 13, we plan to finish up the remaining C++23 features. For progress so far, see the [C++23 Language Features](https://gcc.gnu.org/projects/cxx-status.html#cxx23) table on the [C++ Standards Support in GCC](https://gcc.gnu.org/projects/cxx-status.html) page.  Please do not hesitate to [file bugs](https://gcc.gnu.org/bugs/) in the meantime, and help us make GCC even better!

