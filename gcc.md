# GCC notes

## Links

- [Itanium C++ ABI](http://itanium-cxx-abi.github.io/cxx-abi/)

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

`tparms = <T, U>` (a vector of `TYPE_DECL`s, their types are `TEMPLATE_TYPE_PARM`s)

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

`tparms = <auto>` (a `TYPE_DECL` with type `template_type_parm auto`)

`targs = <>`

- call `unify_one_argument (template_type_parm auto, 42)` -> `unify (template_type_parm auto, int)`
  - both `parm` and 0-th tparm have level 2, because `make_auto_1`: *use a TEMPLATE_TYPE_PARM with a level one deeper than the actual template parms*
  - `parm` has index 0
  - `targs[0] = arg`, so `targs = <int>`

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

## Debuginfo
GCC 11 emits debuginfo for external functions too (*early debug* because of LTO).  This was introduced in [PR96383](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96383), which has unresolved issues: [PR97060](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97060).
E.g.:
```c++
extern void f();  // generates .string "_Z1fv"
int main() {
  f ();
}
```

