# GCC notes

## Links

- [Itanium C++ ABI](http://itanium-cxx-abi.github.io/cxx-abi/)
- [C++ PRs](https://gcc.gnu.org/bugzilla/buglist.cgi?component=c%2B%2B&limit=0&list_id=285895&order=bug_id%20DESC&product=gcc&query_format=advanced&resolution=---)
- [gcc git](https://gcc.gnu.org/git/?p=gcc.git;a=summary)

## C++ front end

### Deducing template arguments from a function call

- `[temp.deduct.call]`
- consider:

```c++
template <typename T, typename U>  // tparms
void fn (T, U) { } // parms

void g () {
  fn (1, "hi"); // no targs, 2 args
}
```

- `fn_type_unification` with `DEDUCE_CALL` -> `type_unification_real` which has a loop and calls `unify_one_argument` for every P/A pair.
Here we have

`parms = [template_type_parm T, template_type_parm U]`

 `args = [1, "hi"]`

`tparms = <T, U>` (a vector of `TYPE_DECL`s, their types are `TEMPLATE_TYPE_PARM`s, this is what we need to deduce)

`targs = <,>` (nothing deduced so far)

1. `unify_one_argument (T, 1)`
    - take the type of the arg = `int`
   - call `unify (T, int)`:
      - `parm` is `TEMPLATE_TYPE_PARM T`, `TEMPLATE_TYPE_LEVEL (parm) == 1`, `TEMPLATE_TYPE_IDX (parm) == 0`
     - check that the level of `T` matches the level of the first template parameter
      - check for mixed types and values, etc.
      - if we already have a targ for this index, we're done
      - otherwise, save `arg` to `targs` for index 0, so `targs = <int,>`


2. `unify_one_argument (U, "hi")`
    - take the type of the arg = `const char[3]`
    - `maybe_adjust_types_for_deduction` will adjust it to `const char *`
   - call `unify (U, const char *)`:
     - `parm` is `TEMPLATE_TYPE_PARM U`, `TEMPLATE_TYPE_LEVEL (parm) == 1`, `TEMPLATE_TYPE_IDX (parm) == 1`
     - save `arg` to `targs` for index 1, so `targs = <int, const char *>`

- `type_unification_real` returns `unify_success`
- now consider
```c++
template <auto>
void fn () { }

void g () {
  fn<42>();
}
```

- `fn_type_unification` -> `do_auto_deduction` -> `type_unification_real`.  Here:


`parms = [template_type_parm auto]`

`args = [42]`

`tparms = <auto>` (a `TYPE_DECL` with type `template_type_parm auto`, this is what we need to deduce)

`targs = <>`

- call `unify_one_argument (template_type_parm auto, 42)` -> `unify (template_type_parm auto, int)`
  - both `parm` and 0-th tparm have level 2, because `make_auto_1`: *use a TEMPLATE_TYPE_PARM with a level one deeper than the actual template parms*
  - `parm` has index 0
  - `targs[0] = arg`, so `targs = <int>`


### Bit-fields
- `TREE_TYPE` is the magic bit-field integral type; the lowered type
- `unlowered_expr_type` is the declared type of the bitfield (uses `DECL_BIT_FIELD_TYPE`)
- related PRs: [PR82165](https://gcc.gnu.org/PR82165), [PR92859](https://gcc.gnu.org/PR92859), [PR87547](https://gcc.gnu.org/PR87547), [PR78908](https://gcc.gnu.org/PR78908), [PR30328](https://gcc.gnu.org/PR30328), [PR65556](https://gcc.gnu.org/PR65556), [PR98043](https://gcc.gnu.org/PR98043)
- e.g.:

```c++
struct S { signed int a:17; } x;
// TREE_TYPE (x.a) = <unnamed-signed:17>
// unlowered_expr_type (x.a) = int
enum E { e0, e1, e2 };
struct R { E a : 2; } y;
// TREE_TYPE (y.a) = <unnamed-unsigned:2>
// unlowered_expr_type (y.a) = E
enum class B { A };
struct C { B c : 8; } z;
// TREE_TYPE (z.c) = signed char
// unlowered_expr_type (z.c) = B
```

- integral promotions: [[conv.prom]](http://eel.is/c++draft/conv.prom)
- unnamed bit-fields are not members

### `finish_call_expr`

- handles `FN (ARGS)` and produces a CALL_EXPR
- if FN is a COMPONENT_REF with a BASELINK:
```c++
return build_new_method_call (object, member, ...);
```
- if FN is a BASELINK, it's a call to a member function:
```c++
result = build_new_method_call (object, fn, args, ...);
```
- if FN is a concept check, call `build_concept_check`
- when we have a functor, FN will be a VAR_DECL; use this to call `operator ()`:
```c++
result = build_op_call (fn, args, complain);
```

- **TODO**

- present in GCC 2.95, in which it only called `build_x_function_call` 

### `VEC_INIT_EXPR`
- already in GCC 2.95, used in `build_new_1`
- represents initialization of an array from another array
- if it represents *value-initialization*, `VEC_INIT_EXPR_VALUE_INIT` will be set
- if it's a potential constant expression, `VEC_INIT_EXPR_IS_CONSTEXPR` will be set
- built by `build_vec_init_expr`.  Used to build a `TARGET_EXPR` too, but not anymore
- `build_vec_init_elt` builds up a single element intialization
- used in `perform_member_init` when initializing an array (see [r165976](https://gcc.gnu.org/git/?p=gcc.git;a=commitdiff;h=534ecb17516c5db7a96245ebb90beb206e22eaff))
- used in a defaulted constructor for a class with a non-static data member of array type
- used in `build_array_copy` when creating a closure object for a lambda
- `VEC_INIT_EXPR` is handled in `cp_gimplify_expr` since [r152318](https://gcc.gnu.org/git/?p=gcc.git;a=commitdiff;h=d5f4edddeb609ad93c7a69ad4575b082de8dc707) -- it uses `build_vec_init` to expand it
- doesn't track whether the initialization was direct-init? [PR82235](https://gcc.gnu.org/PR82235)

### `[with ...]`
- printed by `pp_cxx_parameter_mapping` or `dump_substitution`
- see also `pp_cxx_template_argument_list`
- print the buffer:

`(gdb) p pp.buffer.formatted_obstack`

### `resolve_nondeduced_context`

- given a function template:
```c++
template<typename T>
void foo () { }
```

and `&foo<int>` in the code, `resolve_nondeduced_context` resolves
`&TEMPLATE_ID_EXPR <foo, int>` (type `unknown type`) to `foo` (`ADDR_EXPR` of type `void (*<T358>) (void)`).

- it instantiates `foo` if there are no dependent template arguments
- see [DR 115](https://wg21.link/cwg115)

### `TYPE_USER_ALIGN`
- `finalize_type_size` can (un)set it
- a related [ABI issue](https://gcc.gnu.org/git/?p=gcc.git;a=commitdiff;h=8475f2902a2e2ca5f7ace8bc5265bd1a815dda20)

### `TYPE_PTRMEMFUNC_P`

```c++
struct
{
  void X::<T34e> (struct X *) * __pfn;
  long int __delta;
}
```

### `TYPE_PTRDATAMEM_P`
- is just `OFFSET_TYPE`
- null pointer-to-data-member represented by -1: cf. `null_member_pointer_value_p` and `cp_convert_to_pointer`

### `cp_printer` specs
```
   %A   function argument-list.
   %C   tree code.
   %D   declaration.
   %E   expression.
   %F   function declaration.
   %G   gcall *
   %H   type difference (from).
   %I   type difference (to).
   %K   tree
   %L   language as used in extern "lang".
   %O   binary operator.
   %P   function parameter whose position is indicated by an integer.
   %Q   assignment operator.
   %S   substitution (template + args)
   %T   type.
   %V   cv-qualifier.
   %X   exception-specification.
```

## GCC general
### Reading source files
- after processing the command-line options, we call `cpp_read_main_file` -> `_cpp_find_file` -> `find_file_in_dir` -> `open_file` which does:
```c++
file->fd = open (file->path, O_RDONLY | O_NOCTTY | O_BINARY, 0666);
```
- the source file is read in `read_file` -> `read_file_guts`:
```c++
while ((count = read (file->fd, buf + total, size - total)) > 0)
  {
    // ...
  }
```
- the input buffer is then converted in `_cpp_convert_input` from the input charset to the source character set, if needed.  This can be done using `iconv`.
- then close the file descriptor after reading: `close (file->fd);`
- the buffer is kept in `_cpp_file::buffer`
- this is used for default includes like `stdc-predef.h`, see `cpp_push_default_include`
- then we can actually compile the file: `c_common_parse_file` -> `c_parse_file`

### Reading tokens
- in C++: `cp_lexer_new_main` reads all tokens using `cp_lexer_get_preprocessor_token`:
```c++
  /* Get the remaining tokens from the preprocessor.  */
  while (tok->type != CPP_EOF)
    {
      if (filter)
        /* Process the previous token.  */
        module_token_lang (tok->type, tok->keyword, tok->u.value,
                           tok->location, filter);
      tok = vec_safe_push (lexer->buffer, cp_token ());
      cp_lexer_get_preprocessor_token (C_LEX_STRING_NO_JOIN, tok); 
    }
```
- `cp_lexer_get_preprocessor_token` uses `c_lex_with_flags` -> `cpp_get_token_with_location`
- then we have the tokens saved in `lexer->buffer`: `vec<cp_token, va_gc> *buffer;`
- peek next token: `cp_lexer_peek_token`
- return the next token an consume it: `cp_lexer_consume_token`

## Built-ins

### __builtin_addressof

- used to implement `std::addressof`
- like `&` but works even when the type has an overloaded operator `&` (smart pointers):
```c++
struct S { int operator&() { return 42; } };

int main ()
{
  S s;
  auto p = &s; // invokes operator&
  __builtin_printf ("%d\n", p); // prints 42
  auto q = __builtin_addressof (s);
  __builtin_printf ("%p\n", q); // prints the addr
}
```

## Configure & make
C++:

- run the testsuite with garbage collection at every opportunity:
`make check-g++-strict-gc`
- run the testsuite in all standard conformance levels: `make check-c++-all`

PDP-11: `--target=pdp11-aout`

AARCH64: `--target=aarch64-linux-gnu target_alias=aarch64-linux-gnu`

ARM: `--target=arm-none-eabi`

PPC64LE: `--target=powerpc64le-unknown-linux-gnu`

## Debuginfo
GCC 11 emits debuginfo for external functions too (*early debug* because of LTO).  This was introduced in [PR96383](https://gcc.gnu.org/PR96383), which had unresolved issues: [PR97060](https://gcc.gnu.org/PR97060) (fixed now).
E.g.:
```c++
extern void f();  // generates .string "_Z1fv"
int main() {
  f ();
}
```

## Executable stack
On Linux GCC emits `.note.GNU-stack` sections to mark the code as not needing executable stack; if that section is missing, it's unknown and code linking in such `.o` objects can then make the program require executable stack.  Assembly files need to be marked manually -- e.g. various *.S files in libgcc:
```c++
#if defined(__ELF__) && defined(__linux__)
    .section .note.GNU-stack,"",@progbits
	.previous
#endif
```
