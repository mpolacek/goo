# GCC notes
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
- when we have a functor, FN will be a VAR_DECL, use
```c++
result = build_op_call (fn, args, complain);
```
to call `operator ()`.

- **TODO**

- present in GCC 2.95, in which it only called `build_x_function_call` 



