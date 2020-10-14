# GCC notes

## Links

- [Itanium C++ ABI](http://itanium-cxx-abi.github.io/cxx-abi/)

## C++ front end
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
