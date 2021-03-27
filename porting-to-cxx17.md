# Migrating to C++17

In GCC 11, expected to be released in May 2021, the C++ default was [changed to C++17](https://gcc.gnu.org/gcc-11/changes.html) from C++14; in particular, the `-std=gnu++17` command-line option is now used by default.  C++17 brings a host of [new features](https://gcc.gnu.org/projects/cxx-status.html#cxx17), but it also deprecates, removes, or changes the semantics of certain constructs.  In the following article we take a look at some of the issues users may be facing when switching to GCC 11.  Remember that it is always possible to use the the previous C++ mode by using `-std=gnu++14`.  Moreover, this article only deals with the core language part; deprecated or removed features in the standard C++ library (such as `auto_ptr`) are not discussed here.  I encourage the reader to visit [this document](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0636r3.html) for a broader overview.

### Trigraphs removed
In C++, a trigraph is a sequence of three characters starting with `??` that can express a single character.  For instance, `\` can be written as `??/`.  The reason for this is historical: C and C++ use special characters like `[` or `]` that are not defined in the ISO 646 character set.  The positions of these characters in the ISO table might be occupied by different characters in national ISO 646 characters sets, e.g. `¥` in place of `\`.  Trigraphs were meant to allow expressing such characters, but in practice, they are likely to only be used accidentally, and so were [removed](https://wg21.link/n4086) in C++17.  The removal allows you to play "cute" games like the following test.   Can you see why it works?.  If you for some reason still need to use trigraps in C++17 (indeed, there are code bases which [still use trigraphs](https://wg21.link/n2910)), GCC offers the `-trigraphs` command-line option.

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

In C++14, you can instruct the compiler to warn using `-Wregister`, which ought to help with migrating code to C++17.  Note that GCC still accepts explicit register variables without warning:

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

TODO

* [ ] 
C++17 changes to template template parameter matching can be disabled independently of other features with -fno-new-ttp-matching.  DR 150 P0522R0 r243871

* [ ]
the evaluation order rules are different in C++17,

* [ ]
static constexpr data members are now implicitly inline (which makes
  them definitions),

* [ ]
C++17 requires guaranteed copy elision, meaning that a copy/move
  constructor call might be elided completely.  That means that if
  something relied on a constructor being instantiated via e.g. copying
  a function parameter, it might now fail.
