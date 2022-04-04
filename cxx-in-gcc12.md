*WIP*

# New C++ features in GCC 12

The GNU Compiler Collection (GCC) 12.1 is expected to be released in April 2022.  Like every major GCC release, this version will bring many [additions, improvements, bug fixes, and new features](https://gcc.gnu.org/gcc-12/changes.html).  GCC 12 is already the system compiler in [Fedora 36](https://fedoraproject.org/wiki/Changes/GNUToolchainF36).  GCC 12 will also be available on [Red Hat Enterprise Linux](https://developers.redhat.com/products/rhel/overview) in the Red Hat Developer Toolset (RHEL 7), or the Red Hat GCC Toolset (RHEL 8 and 9).

As the similar blog post I wrote about [GCC 10](https://developers.redhat.com/blog/2020/09/24/new-c-features-in-gcc-10), this article describes new features only in the C++ front end.

We implemented several C++23 proposals in GCC 12.  The default dialect in GCC 12 is `-std=gnu++17`; to enable C++23 features, use the `-std=c++23` or `-std=gnu++23` command-line option. (Note that the latter option allows GNU extensions.) 

## C++23 features

### `if consteval`

C++17 introduced the constexpr if statement.  The condition in `constexpr if` must be a constant expression (it is *manifestly constant evaluated*).  If the condition evaluates to true, the "else" branch, if present, is discarded (which means, for instance, that the "else" branch will not be instantiated, which is a different behavior from an ordinary `if`).

C++20 then introduced the `consteval` keyword.  A (possibly member) function or a constructor marked as `consteval` is an immediate function.  Immediate functions must produce a constant when they are called (unless the call to an immediate function takes place in another immediate function), if they don't, the compiler will produce an error.  The compiler will not emit any actual code for such functions.

In contrast, a `constexpr` function may or may not be evaluated at compile-time -- depending on the context.  To that end, C++20 introduced a new library function `std::is_constant_evaluated()`, which returns true if the current context is "evaluated at compile time":

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

There is a problem with the interaction between `if constexpr` and `std::is_constant_evaluated()`, but fortunately the compiler can  detect that problem.  See the *Extended `std::is_constant_evaluated` in `if` warning* paragraph below.

### `auto(x)`

### Non-literal variables in constexpr functions

### Multidimensional subscript operator

XXX when done

### `elifdef` and `elifndef`

First in C

### Extended *init-statement*

allow *alias-declaration*

### auto specifier for pointers and references to arrays

Note: You can find the overall defect resolution status on the [C++ Defect Report Support in GCC](https://gcc.gnu.org/projects/cxx-dr-status.html) page.

## Other changes

### DR 2374, Overly permissive specification of `enum` direct-list-initialization

GCC 12 implements this defect report, which forbids, for instance,
direct-list-initialization of a scoped enumeration from a different
scoped enumeration:

```c++
enum class Orange;
enum class Apple;
Orange o;
Apple a{o}; // error with GCC 12
```

### DR 1315, Restrictions on non-type template arguments in partial specializations 

This is now accepted:

```c++
template <int I, int J> struct A {};
template <int I> struct A<I, I*2> {};
```

### DR 1227, Substitute into function parms in lexical order

XXX b0d73a66ae3962fa83309527d85613d72a6aa43d

### Stricter checking of attributes on friend declarations

If a friend declaration has an attribute, that declaration must be a definition.
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

XXX //gcc.gnu.org/PR79501

```c++
struct X {
  template<typename T> struct A {};
  A() -> A<int>;
};
```

### Ordered comparison of null pointers is now rejected

Relational comparisons between null pointer constants and pointers are
ill-formed and this is diagnosed in GCC 12:

```c++
decltype(nullptr) foo ();
auto cmp = foo () > 0; // error: ordered comparison of pointer with integer zero
```

## New and improved warnings

### `-Wuninitialized` extended

XXX

### `-Wbidi-chars` added

XXX 51c500269bf53749b107807d84271385fad35628

### `-Warray-compare` added

XXX 2dda00b734888d3b53ac91160083b5c6cd5ca5c8

### `-Wno-attributes=ns::attr`

XXX a1ad0d84d7fcbcaa7a697290387b911eb7e1bd46

### New warning options for C++ language mismatches

XXX ee336ecb2a7161bc28f6c5343d97870a8d15e177
This adds new warning flags, enabled by default: -Wc++11-extensions,
-Wc++14-extensions, -Wc++17-extensions, -Wc++20-extensions, and
-Wc++23-extensions.  can be used to control existing pedwarns about
occurences of new C++ constructs in code using an old C++ standard
dialect.

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

### Missing `requires` warning

XXX e18e56c76be35e6a799e07a01c24e0fff3eb1978

### `-Waddress` enhanced

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

## Conclusion
In GCC 13, we plan to finish up the remaining C++23 features. For progress so far, see the [C++23 Language Features](https://gcc.gnu.org/projects/cxx-status.html#cxx23) table on the [C++ Standards Support in GCC](https://gcc.gnu.org/projects/cxx-status.html) page.  Please do not hesitate to [file bugs](https://gcc.gnu.org/bugs/) in the meantime, and help us make GCC even better! 
