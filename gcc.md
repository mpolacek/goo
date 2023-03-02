# GCC notes

## Links

- [Itanium C++ ABI](http://itanium-cxx-abi.github.io/cxx-abi/)
- [C++ PRs](https://gcc.gnu.org/bugzilla/buglist.cgi?component=c%2B%2B&limit=0&list_id=285895&order=bug_id%20DESC&product=gcc&query_format=advanced&resolution=---)
- [gcc git](https://gcc.gnu.org/git/?p=gcc.git;a=summary)
- [gcc lcov](https://splichal.eu/lcov/index.html)
- [safe bool problem](https://en.cppreference.com/w/cpp/language/implicit_conversion)

## C++ front end
### Constant expressions

Certain contexts require constant expressions.  Constant expressions can be evaluated during translation.

constant expression
: either a glvalue core constant expression that refers to an entity that is a permitted result of a constant expression, or a prvalue core constant expression whose value satisfies the following constraints: *[expr.const]p11*

core constant expression
: an expression E whose evaluation does not evaluate one of the following: see *[expr.const]p5*

converted constant expression
: an expression, implicitly converted to type T, where the converted expression is a constant expression and the implicit conversion sequence contains only *[expr.const]p10*

contextually converted constant expression of type `bool`
: an expression, contextually converted to `bool`, where the converted expression is a constant expression and the conversion sequence contains only the conversions above (*[expr.const]p10*)

manifestly constant-evaluated expression
: see *[expr.const]p14*

potentially constant evaluated expression
: see *[expr.const]p15*

- we require a *constant-expression* in these contexts:
  - `case e`
       - `finish_case_label` -> `case_conversion` -> `cxx_constant_value`
  - `static_assert(e)`
    - `finish_static_assert` -> `fold_non_dependent_expr`
  - `arr[e]`
    - `compute_array_index_type_loc` -> `fold_non_dependent_expr`
  - `explicit(e)`
    - `build_explicit_specifier` -> `cxx_constant_value`
  - `noexcept(e)`
    - `build_noexcept_spec` -> `cxx_constant_value`
  - *enumerator* `= e`
    - `build_enumerator` -> `cxx_constant_value`
  - `alignas(e)`
    - `cxx_alignas_expr` -> `cxx_constant_value`
  - `btfld : e` (a *member-declaration*)
    - `check_bitfield_decl` -> `cxx_constant_value`
  - *template-argument*
    - `convert_nontype_argument` -> `maybe_constant_value`
- to evaluate a constant expression, we use functions like
  - `maybe_constant_value`
  - `cxx_constant_value`
  - `maybe_constant_init`
  - `cxx_constant_init`
  - `fold_non_dependent_expr`
  - `fold_non_dependent_init`
- the core of constexpr evaluation is `cxx_eval_outermost_constant_expr` (more below)
- as an aside, other fold functions in the C++ FE:
  - `fold_simple`
    - only few simplifications like `FLOAT_EXPR` -> `REAL_CST`, or fold `SIZEOF_EXPR`s
    - can call middle-end `const_unop`
  - `fold_for_warn`
  - `cp_fully_fold`
  - `cp_fully_fold_init`
  - `cp_fold_function`
- we now use the pre-generic form for constexpr evaluation

#### `potential-constant-expression?`
To check if an expression could be constant, you can use:

- `potential_constant_expression`
- `potential_rvalue_constant_expression`
- `require_potential_constant_expression`
- `require_potential_rvalue_constant_expression`
- `require_rvalue_constant_expression`
- `is_constant_expression`
- `is_rvalue_constant_expression`
- `require_constant_expression`
- `is_static_init_expression`
- `is_nondependent_constant_expression`
- `is_nondependent_static_init_expression`

There are various parameters to specify the behavior: if we want to imply an lvalue-rvalue conversion, if an error message should be emitted, whether we want to consider the constexpr context, or if we should be strict (affects `VAR_DECL`s).  For instance, a `reinterpreter_cast` is not a potential constant expression.  Or a temporary of non-literal type is also not a constant expression.

#### `maybe_constant_value`
- takes a (non-dependent) constant expression and returns its reduced value
- noop for `CONSTANT_CLASS_P` expressions
- doesn't error on a non-constant expression; is `strict`
- unlike the similar functions, uses a hash table to store evaluated expressions: `tree -> tree` mapping in `cv_cache`
- if the expression to evaluate isn't already in the hash table, call `cxx_eval_outermost_constant_expr` to actually evaluate the expression (see below)

#### Data structures in constexpr
##### `constexpr_global_ctx`
- a constexpr expansion context, just one per evaluate-outermost-constant-expression
- most importantly, contains a hash map of temporaries or local variables within the constant expression: `hash_map<tree,tree> values;`
- created only in `cxx_eval_outermost_constant_expr`, once per each call

##### `constexpr_ctx`
- the constexpr expansion context, we can have more of them per one "main" constexpr expansion context
- we create a new one when evaluating a function call, array reference, a `{}`, ... see `cxx_eval_builtin_function_call`, `cxx_eval_call_expression`, `cxx_eval_array_reference`, `cxx_eval_bare_aggregate`, `cxx_eval_vec_init_1`
- contains e.g. the innermost call we're evaluating; the constructor we're currently building up (in aggregate initialization); a pointer to its parent; a pointer to the global constexpr context; `quiet`/`strict`/`manifestly_const_eval` flags

##### `constexpr_call`
- represent calls to constexpr functions
- contains e.g. the function definition, the mapping of its parameters, and the result of the function call

#### `cxx_eval_outermost_constant_expr`
- the central point to evaluate a constexpr
1. create a new `constexpr_global_ctx` and `constexpr_ctx`
2. if we're evaluating an aggregate, create a new `{}` and put it into `ctx.ctor`, set `ctx.object` (which is what we're building the CONSTRUCTOR for)
3. instantiate any constexpr functions named in the expression (P0859, see `instantiate_constexpr_fns`)
4. actually evaluate the expression: `cxx_eval_constant_expression`
5. make sure that we got a constant expression; perform some checks.  In constexpr, when an expression couldn't be evaluated, we set `non_constant_p`.
6. unshare the evaluated expression
7. return the evaluated expression

#### `cxx_eval_constant_expression`
- this is where we perform the actual constexpr evaluation
- depends if we want an lvalue or not (for instance, when evaluating `&exp` we want an lvalue `exp`)
- `ctx->strict` makes a difference for `VAR_DECL`s
- depending on the `TREE_CODE` of the expression dispatches to more specialized functions, e.g. `cxx_eval_loop_expr` et al
- does ***not*** handle template codes; those need to be instantiated first via `fold_non_dependent_expr` and such.  Otherwise you'll get "unexpected expression of kind".

#### Example #1
Consider
```c++
constexpr int g = 42;
constexpr int foo (int i) {
  int r = i * 2;
  return r + g;
}
static_assert (foo (4) == 50);
```

1. `finish_static_assert` gets as the condition `foo (NON_LVALUE_EXPR <4>) == 50`.  Use `fold_non_dependent_expr` -> `maybe_constant_value` to evaluate it.  We're in a `manifestly_const_eval` context: [expr.const]p14: *An expression or conversion is manifestly constant-evaluated if it is: -- a constant-expression, [...]* and
```
static_assert-declaration:
  static_­assert ( constant-expression ) ;
  static_­assert ( constant-expression , string-literal ) ;
```
So we don't look into the `cv_cache` and go straight to...

2. `cxx_eval_outermost_constant_expr`: build up a new global and "local" constexpr context.  The expression doesn't contain any constexpr functions to instantiate.
3. call `cxx_eval_constant_expression` to actually evaluate the expression.  We have an `EQ_EXPR` -> call `cxx_eval_binary_expression` on both the LHS and RHS.  The RHS is 50, so there's nothing to do.  The LHS is a `CALL_EXPR`, so `cxx_eval_constant_expression` calls:
4. `cxx_eval_call_expression (foo (NON_LVALUE_EXPR <4>))`:
   1. new `constexpr_call`
   2. lookup the function definition foo in `constexpr_fundef_table` using `retrieve_constexpr_fundef` (it is put there by `register_constexpr_fundef`): 
`(gdb) p *new_call.fundef
$34 = {decl = <function_decl 0x7fffea232e00 foo>, body = <bind_expr 0x7fffea241990>, 
  parms = <parm_decl 0x7fffea242100 i>, result = <result_decl 0x7fffea0dde88>}`

    3. evaluate the function call arguments: `cxx_bind_parameters_in_call` walks over the function arguments (here it's `(4)`) and evaluates them via `cxx_eval_constant_expression` and then saves them into the vector `new_call->bindings`.  Here that results in `<4>`.
 
    4. look to see if we've already evaluated this call before with the same parameter bindings:
`(gdb) p new_call
$47 = {fundef = 0x7fffea22cb20, bindings = <tree_vec 0x7fffea22cc40>, result = <tree 0x0>, 
  hash = 4171003643, manifestly_const_eval = true}`

    5. since we haven't evaluated this call before, we need to do so now.  Start by remapping the parameters: take our `new_call.bindings` which is `<4>` and for each element of this vector:
       - unshare it,
       - put it into the hash map: `ctx->global->values.put (remapped, arg);` where `remapped` is `parm_decl i` and `arg` is `4`. 
        - put the function's `RESULT_DECL` into the map: `ctx->global->values.put (res, NULL_TREE);` (we have no value for it yet)
       - evaluate the function's body: `cxx_eval_constant_expression (body)` where `body` is

```c++
int r = VIEW_CONVERT_EXPR<int>(i) * 2;
return <retval> = VIEW_CONVERT_EXPR<int>(r) + (int) VIEW_CONVERT_EXPR<const int>(g);
```

- note that right before evaluating the body we get

```
(gdb) p *ctx->global->values.get(res)
$71 = (tree_node *) 0x0
```
but after it:
```
(gdb) pge *ctx->global->values.get(res)
50
```
which is the result:
```c++
result = *ctx->global->values.get (res);
```
   
- now remove the `RESULT_DECL` and parameter binding from the hash map
- save the result to our `constexpr_call.result` field: `entry->result = cacheable ? result : error_mark_node;`
and we're done: `return result;`

5. so the result of the call to `foo (4)` is `50`.  Now we're back in `cxx_eval_binary_expression` and ready to get the result and return it:

```
(gdb) p code
$78 = EQ_EXPR
(gdb) pge lhs
50
(gdb) pge rhs
50
3188	    r = fold_binary_loc (loc, code, type, lhs, rhs);
```

6. `maybe_constant_value` returns `1`

#### Other
- `constexpr` is Turing-complete after [DR 1454](https://wg21.link/cwg1454) (reference parameters in constexpr functions)
- `*_constant_init`: `strict` is false (we try to get constant values for more than just C++ constant expressions)
- `*_constant_value`: `strict` is true
- `build_date_member_initialization` --- DM initialization in constexpr constructor; builds up a pairing of the data member with its initializer
- `__builtin_is_constant_evaluated` --- [P0595](https://wg21.link/p0595)
- [P0859](https://wg21.link/p0859): a function is needed for constant evaluation if it is a `constexpr` function that is named by an expression that is potentially constant evaluated -> so we need to instantiate any `constexpr` functions mentioned by the expression:
  - `instantiate_constexpr_fns` has `cp_walk_tree` with `instantiate_cx_fn_r`, checks `DECL_TEMPLOID_INSTANTIATION` and calls `instantiate_decl` for constexpr function decls
- can't call `maybe_constant_value` on an unresolved overloaded function (like in [PR46903](https://gcc.gnu.org/PR46903))
- N4268: Allow constant evaluation for all non-type template arguments
- in a constexpr function, a parameter is potentially constant when evaluating a call to that function, but it is not constant during parsing of the function; see this [patch](https://gcc.gnu.org/pipermail/gcc-patches/2020-May/546260.html)
- `is_instantiation_of_constexpr` -- if a function is an instantiation of a constexpr function
- `cp_function_chain->invalid_constexpr` -- set for invalid constexpr functions
- since [this](https://gcc.gnu.org/git/?p=gcc.git;a=commitdiff;h=1595fe44e11a969d8ae462212886fb0a87b46226) we limit the initial instantiation of all used functions to manifestly-constant-evaluated expressions:
```
-  instantiate_constexpr_fns (r);
+  if (manifestly_const_eval)
+    instantiate_constexpr_fns (r);
```

### Parser
Life begins in `c_parse_file`:

- create a new main C++ lexer and new C++ parser
- call `cp_parser_translation_unit`
- call `finish_translation_unit`

`cp_parser_translation_unit`:

- handle *[gram.basic]*
- call `cp_parser_toplevel_declaration` until EOF

- [ ] [PR64666](https://gcc.gnu.org/PR64666) - we accept an assignment in a constant-expression but we shouldn't; see `cp_parser_constant_expression`

#### Delayed parsing
[class.mem]p7: A *complete-class context* of a class is:

- function body
- default argument
- default template argument
- *noexcept-specifier*
- default member initializer

within the member-specification of the class or class template.

And [class.mem]p8: *The class is regarded as complete within its complete-class contexts.*  So we defer parsing to handle this.  This is implemented via `cp_parser_late_parsing_for_member`, `cp_parser_late_parsing_nsdmi`, `cp_parser_late_noexcept_specifier`, etc.  An unparsed entity is represented by a `DEFERRED_PARSE`.


### Templates

`TEMPLATE_DECL` ~ represents a template definition

| accessor | |
| - | - |
| `DECL_ARGUMENTS` | template parameter vector |
| `DECL_TEMPLATE_INFO` | template info |
| `DECL_VINDEX` | list of instantiations (functions only) |

| class templates | |
| - | - |
| `DECL_INITIAL` | associated templates |

| non-class templates | |
| - | - |
| `TREE_TYPE` | type of object to be constructed |
| `DECL_TEMPLATE_RESULT` | decl for object to be created (e.g., the `FUNCTION_DECL` |


#### `tsubst`
- deals with types, decls, ...
- `DECL_P` -> `tsubst_decl`
- for some things like `integer_type_node` or `NULLPTR_TYPE` just returns the tree
- for `INTEGER_TYPE`: substitute `TYPE_MAX_VALUE`
- handles e.g.: `TEMPLATE_TYPE_PARM`, `TREE_LIST`, `TREE_VEC`, `POINTER_TYPE`, `FUNCTION_TYPE`, `DECLTYPE_TYPE`

#### `tsubst_copy`
- does the substitution but doesn't finish the expression
- also deals with `_DECL`

#### `tsubst_expr`
- like `tsubst_copy` for expressions, does semantic processing
- `RETURN_EXPR`: calls `finish_return_stmt` after recursing on its operands
- `EXPR_STMT`, `USING_STMT`, `DECL_EXPR`, `BIND_EXPR`, ...
- calls `finish_*`

#### `tsubst_copy_and_build`
- deals with expressions and performs semantic analysis
- `IDENTIFIER_NODE`: `finish_id_expression`
- `TEMPLATE_ID_EXPR`
  - `tsubst_template_args`, `lookup_template_function`
  - builds a `COMPONENT_REF`
- `INDIRECT_REF`: `tsubst` + `build_nop`
- `IMPLICIT_CONV_EXPR`: `tsubst` + recurse on the operand + `perform_implicit_conversion`
- handles various cast expressions
- `++`, `--`, binary ops
- `CALL_EXPR`: depends if it's ADL or not; `finish_call_expr`
- `COND_EXPR`
- handles a `CONSTRUCTOR` which `tsubst` doesn't
- no `tsubst*` function handles `AGGR_INIT_EXPR`

#### non-deduced contexts
- see *[temp.deduct.type]*, e.g., *the expression of a decltype-specifier*
- in `unify`:
```c++
    case TYPEOF_TYPE:
    case DECLTYPE_TYPE:
    case UNDERLYING_TYPE:
      /* Cannot deduce anything from TYPEOF_TYPE, DECLTYPE_TYPE,
         or UNDERLYING_TYPE nodes.  */
      return unify_success (explain_p);
```

- the operand of `decltype` is also an unevaluated operand -> NS class members might be named

#### `tf_partial`

- when doing initial explicit argument substitution in `fn_type_unification`.  E.g.,
```c++
template<typename T, typename U>
T fn (T t, U u)
{
  return t + u;
}

void g ()
{
  fn<int>(1, 1);
}
```
in `fn_type_unification`: we provided the first template argument:

`explicit_targs` = `<int>`

`fntype` = `T <T36d> (T, U)`

and `tf_partial` is used:
```c++
/* Adjust any explicit template arguments before entering the
   substitution context.  */
explicit_targs
  = (coerce_template_parms (tparms, explicit_targs, fn,
                            complain|tf_partial,
                            /*require_all_args=*/false,
                            /*use_default_args=*/false));
```
and when substituting the explicit args into the function type.  See [temp.deduct.general]/2: *When an explicit template argument list is specified ... the specified template argument values are substituted for the corresponding template parameters.*
```c++
tsubst_flags_t ecomplain = complain | tf_partial | tf_fndecl_type;
fntype = tsubst (TREE_TYPE (fn), explicit_targs, ecomplain, NULL_TREE);
```
so now `fntype` is `int <T36e> (int, U)`.  [temp.deduct.general]/5: *The resulting substituted and adjusted function type is used as the type of the function template for template argument deduction.* To deduce `U`:

```c++
  // full_targs = int,
  // parms = int, U, void
  // args = 1
  // DECL_INNERMOST_TEMPLATE_PARMS (fn) = T, U
  ok = !type_unification_real (DECL_INNERMOST_TEMPLATE_PARMS (fn),
                               full_targs, parms, args, nargs, /*subr=*/0,
                               strict, &checks, explain_p);
  // full_targs = targs = int, int
```
Also: *When all template arguments have been deduced or obtained from default template arguments, all uses of template parameters in the template parameter list of the template are replaced with the corresponding deduced or default argument values.*

and

*If type deduction has not yet failed, then all uses of template parameters in the function type are replaced with the corresponding deduced or default argument values.*

[temp.deduct.general]/6: *At certain points in the template argument deduction process it is necessary to take a function type that makes use of template parameters and replace those template parameters with the corresponding template arguments.
This is done at the beginning of template argument deduction when any explicitly specified template arguments are substituted into the function type, and again at the end of template argument deduction when any template arguments that were deduced or obtained from default arguments are substituted.*

- also note that *The equivalent substitution in exception specifications is done only when the noexcept-specifier is instantiated, at which point a program is ill-formed if the substitution results in an invalid type or expression*.

- now
```c++
// fn = template_decl fn
// targs = int, int
decl = instantiate_template (fn, targs, complain);
// decl = function_decl fn
// TREE_TYPE (decl) = int <T112> (int, int)

// ...

return r; // r == decl
```

- in [PR89966](https://gcc.gnu.org/PR89966) passing `tf_partial` prevented `do_auto_deduction` from actually replacing `auto`, so `do_auto_deduction` now clears `tf_partial`

#### `retrieve_specialization`
- tries to find a specialization (either an instantiation or an explicit specialization) of a template with certain arguments
- uses the `decl_specializations` and `type_specializations` hash tables (unless `optimize_specialization_lookup_p`)
- can use `register_specialization` or `reregister_specialization` to add a specialization
- e.g.,
```c++
gen_tmpl = most_general_template (tmpl);
// gen_tmpl e.g. "template<class U> template<class V> struct B<U>::C"
// args e.g. <int, V>
spec = retrieve_specialization (gen_tmpl, argvec, hash);
```

#### `CLASSTYPE_USE_TEMPLATE`

- if a tree is a specialization of a template.  This is for ordinary explicit specialization and partial specialization of a template class such as:

```c++
template <> class C<int>;
```
or
```c++
template <class T> class C<T*>;
```

- =  1 implicit instantiation: `CLASSTYPE_IMPLICIT_INSTANTIATION`
- = 2 partial or explicit specialization: `CLASSTYPE_TEMPLATE_SPECIALIZATION`
- = 3 explicit instantiation: `CLASSTYPE_EXPLICIT_INSTANTIATION`
- 1 or 3: `CLASSTYPE_TEMPLATE_INSTANTIATION`
- in debug output: `use_template=[01]`
- `lookup_template_class` sets `CLASSTYPE_IMPLICIT_INSTANTIATION` for a partial instantiation (i.e., for the type of a member template class nested within a template class); required for `maybe_process_partial_specialization` to work correctly.
- to get template arguments from a `RECORD_TYPE`, e.g. `std::pair<const int&, const int&>`: use `CLASSTYPE_TI_ARGS`.  The template parameters of `std::pair`, that is `template<typename _T1, typename _T2>` can be obtained by using `DECL_TEMPLATE_PARMS`.

#### `PRIMARY_TEMPLATE_P`
- a variable in a function template will get a `TEMPLATE_DECL` (created by `start_decl` -> `push_template_decl` -> `build_template_decl`), its `DECL_TEMPLATE_RESULT` will be a `VAR_DECL`, so `variable_template_p` checks for `PRIMARY_TEMPLATE_P`:
```c++
template<typename> void f() {
  extern int foo;
  foo = 1; // gets a TEMPLATE_DECL, its DECL_PRIMARY_TEMPLATE
           // is TEMPLATE_DECL f, so it's not PRIMARY_TEMPLATE_P
}
```

#### Deducing template arguments from a function call

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

#### Dependent names
Some expressions are never dependent: [[temp.dep.expr]p4](http://eel.is/c++draft/temp.dep.expr#4): *literal*, `sizeof`, `typeid`, `noexcept(expr)`, `alignof`

#### Injected class names
```c++
template<typename T>
struct A {
  void f() { T::template foo<A>(); }
};
```
the template argument for `T::foo` might be a type or a template

[DR 176](https://wg21.link/cwg176), makes it easier to refer to the current instantiation

#### `redeclare_class_template`
- called for e.g.
```c++
template<class T> struct S;
template<class T> struct S { };
```
to detect things like
```c++
template<class = int> class C;
template<class = int> class C { }; // error: redefinition of default argument
```
though we don't do it for function templates -- bug!

#### Other

- `add_template_candidate_real` --- if `TMPL` can be successfully instantiated by the given template arguments and the argument list, add it to the candidates; see [temp.over]
- template-id aren't reparsed: if `cp_parser_template_id` sees `CPP_TEMPLATE_ID`, it uses `saved_checks_value`
- `lookup_template_function` --- returns a `TEMPLATE_ID_EXPR` corresponding to the function + arguments
  - if it's `BASELINK_P`, create a `TEMPLATE_ID_EXPR` into its `BASELINK_FUNCTIONS`
  - if it has no type/is `OVERLOAD`: use `unknown_type_node`
- `pending_templates` --- a list of templates whose instantiations have been deferred
  - `instantiate_pending_templates`, `add_pending_template`
- `DECL_FUNCTION_TEMPLATE_P` -- if a `TEMPLATE_DECL` is a function template.  Functions templates cannot be partially specialized, checked in `check_explicit_specialization`
- `current_template_args` -- within the declaration of a template, return the currently active template parameters; is a vector
- `template_parm_scope_p` -- if this scope was created to store template parameters
- `begin_specialization`-- after seeing `template<>`
- [DR 727](https://wg21.link/cwg727): [temp.expl.spec]: *An explicit specialization may be declared in any scope in which the corresponding primary template may be defined.*  We don't implement it yet.
  - `check_specialization_namespace`, `check_explicit_instantiation_namespace`, `check_explicit_specialization`
- class templates: member functions are instantiated only if they are used
- if a class template has static members, they are instantiated once for each type for which the class template is used
- passing instantiated codes to `tsubst` -> crash; see e.g. `case FIX_TRUNC_EXPR` in `tsubst_copy_and_build`
  - we used to handle (wrongly) `FIX_TRUNC_EXPR` in `tsubst_copy_and_build`, but since [r258821](https://gcc.gnu.org/git/?p=gcc.git;a=commitdiff;h=c1e7c3f2015247369b040a3ab24e85d4d68f51f4) we don't
- deduction against `braced-init-list` wasn't supported until [DR 1591](https://wg21.link/cwg1591)
- [DR 226](https://wg21.link/cwg226): allowed template default arguments of function templates (C++11)
- `decl_constant_var_p` -- if the VAR_DECL's value can be used in a constant expression.  Calls `maybe_instantiate_decl (decl)` to detect using DECL in its own initializer.
- `check_template_shadow` -- checks if a decl shadows a template parameter
- `build_value_init` doesn't work in templates
- `build_vec_init` in a template builds up a lot of garbage that we'll throw away at the end (see [PR93676](https://gcc.gnu.org/PR93676))

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

### `build_over_call`
- has the `CHECKING_P` check in C++17: we check to make sure that we are initializing directly from class prvalues rather than copying them:
```c++
/* In C++17 we shouldn't be copying a TARGET_EXPR except into a
   potentially-overlapping subobject.  */
if (CHECKING_P && cxx_dialect >= cxx17)
    gcc_assert (TREE_CODE (arg) != TARGET_EXPR
                || force_elide
                /* It's from binding the ref parm to a packed field. */
                || convs[0]->need_temporary_p
                || seen_error ()
                /* See unsafe_copy_elision_p.  */
                || unsafe_return_slot_p (fa));
```

### `TARGET_EXPR`
- `INIT_EXPR` with a `TARGET_EXPR` as the RHS = direct-init
- `MODIFY_EXPR` with a `TARGET_EXPR` as the RHS = copy
- for classes but also when converting an integer to a reference type: `convert_like_internal/ck_ref_bind`
- can express direct-init: `TARGET_EXPR_DIRECT_INIT`
- `TARGET_EXPR_DIRECT_INIT_P` means there 
isn't a temporary involved (-> checking in `cp_gimplify_expr` that we don't see any `TARGET_EXPR` with that 
flag set (because they all get folded away by `cp_gimplify_init_expr`)
- should never get into `fold_non_dependent_expr`

### `NON_DEPENDENT_EXPR`
- introduced 2003-07-08, gcc-3.4.0, [r69130](https://gcc.gnu.org/git/?p=gcc.git;a=commitdiff;h=d17811fd1aad24d0f47d0b20679753b23803848b)
- type-computation for non-dependent expressions
- `build_non_dependent_expr`
- a placeholder for an expression that is not type-dependent, but does occur in a template
- the standard says that we have to compute the types of expressions in templates where possible:
```c++
struct A { template<int I> void f(); };
struct B { int f; };
A *g(int);
B *g(double);
template<typename T> void h() { g(2)->f<3>(); }
```
`f<int>` is a template.  We have to get the type of `g(2)` in order to parse the expr.

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

### `PLACEHOLDER_EXPR`
- used when we don't have a `this` parameter to refer to yet
- consider:

```c++
struct S {
  int a;
  void *p = this; // #1
};

void g()
{
  S a{1}; // #2
}
```

- `get_nsdmi` creates a `PLACEHOLDER_EXPR` for `S::p`'s NSDMI (`#1`), so its initializer is `(void *) &<PLACEHOLDER_EXPR struct S>`.
- after we've created a `VAR_DECL` for `a`, `store_init_value` -> `replace_placeholders` gets `exp={.a=1, .p=(void *) &<PLACEHOLDER_EXPR struct S>}, obj=a`, replaces the PLACEHOLDER_EXPR, as the name suggests, and returns `{.a=1, .p=(void *) &a}`

### `CONSTRUCTOR_PLACEHOLDER_BOUNDARY`
- used so that `replace_placeholders_r` doesn't walk into constructors that have `PLACEHOLDER_EXPR`s related to another object.  E.g.,

```c++
struct C {};
struct X {
  unsigned i;
  unsigned n = i;
};

C bar (X x)
{
  return {};
}

int main ()
{
  C c = bar (X {1});
}
```

- the init for `n` is  initially `((struct X *) this)->i`, but we don't have the object yet, so we create a `PLACEHOLDER_EXPR` and the init is then `(&<PLACEHOLDER_EXPR struct X>)->i` (cf. `get_nsdmi`)
-  `CONSTRUCTOR_PLACEHOLDER_BOUNDARY` is set on `{NON_LVALUE_EXPR <1>}`: `process_init_constructor_record` is processing the initializer `{NON_LVALUE_EXPR <1>}` for `X`, it walks all members of `X` and it sees that the NSDMI for `n` (which is `(&<PLACEHOLDER_EXPR struct X>)->i`) has a `PLACEHOLDER_EXPR`
-  `{NON_LVALUE_EXPR <1>}` is turned into `{.i=1, .n=(&<PLACEHOLDER_EXPR struct X>)->i}` in `process_init_constructor_record`
- then `replace_placeholders_r` will not walk into the constructor when it's called from `store_init_value` with `exp=bar (TARGET_EXPR <D.2396, {.i=1, .n=(&<PLACEHOLDER_EXPR struct X>)->i}>), obj=c`.  If it did, we'd crash, because we'd attempt to substitute a `PLACEHOLDER_EXPR` of type `X` with an object of type `C`.  (Note that here `*t != d->exp`, see below.  `d->exp` is the whole `bar(...)` call, `*t` only the constructor sub-expression.)
- the `PLACEHOLDER_EXPR X` is replaced with `D.2432` in `cp_gimplify_init_expr` which gets `D.2432 = {.i=1, .n=(&<PLACEHOLDER_EXPR struct X>)->i}`.  The `CONSTRUCTOR_PLACEHOLDER_BOUNDARY` is still set, but this time in:

```c++
        if ((CONSTRUCTOR_PLACEHOLDER_BOUNDARY (*t) 
             && *t != d->exp)
            || d->pset->add (*t))
```

`*t` actually is `d->exp` (= the outermost expression).

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

### Alias
1. *type alias*

 - like `typedef`
```
alias-declaration:
  using identifier attribute-specifier-seq [opt] = defining-type-id ;
```
2. *alias template*

- a *template-declaration in which the declaration is an alias-declaration*
- refers to a family of types
- [temp.alias]

#### More on aliases
- `using`/`typedef` represented by a `TYPE_DECL`
- `using name = type` has `TYPE_DECL_ALIAS_P` set
- can look at its `DECL_NAME` and `DECL_ORIGINAL_TYPE`

#### `TEMPLATE_DECL_COMPLEX_ALIAS_P`
- introduced in [here](https://gcc.gnu.org/pipermail/gcc-patches/2015-June/421158.html)
- used in `dependent_alias_template_spec_p`

### Mangling

#### Destructors
- `D0`, `D1`, ..., see `write_special_name_destructor`:

```c++
  if (DECL_DELETING_DESTRUCTOR_P (dtor))
    write_string ("D0");
  else if (DECL_BASE_DESTRUCTOR_P (dtor))
    write_string ("D2");
  else if (DECL_MAYBE_IN_CHARGE_DESTRUCTOR_P (dtor))
    /* This is the old-style "[unified]" destructor.
       In some cases, we may emit this function and call
       it from the clones in order to share code and save space.  */
    write_string ("D4");
  else 
    {    
      gcc_assert (DECL_COMPLETE_DESTRUCTOR_P (dtor));
      write_string ("D1");
    }
```

- for `D4`, see e.g. [this](https://gcc.gnu.org/legacy-ml/gcc-patches/2013-11/msg02724.html)

### Other
- `grok_op_properties` --- checks a declaration of an overloaded or conversion operator
- `grok_ctor_properties` --- checks if a constructor has the correct form
- `grok_reference_init` --- handles initialization of references
- `grok_special_member_properties` --- sets `TYPE_HAS_*` flags like `TYPE_HAS_USER_CONSTRUCTOR`
- `AGGR_INIT_EXPR` --- useful where we want to defer actually building up the code to manipulate the object until we know which object it is we're dealing with
- `simplify_aggr_init_expr` --- `AGGR_INIT_EXPR` --> `CALL_EXPR`
- `defaultable_fn_check` --- if a function can be explicitly defaulted
- `convert_nontype_argument` --- follows [temp.arg.nontype]; called twice for each targ = double coercion (`finish_template_type` + `instantiate_template_class`)
- `do_class_deduction` --- C++17 class deduction
- `do_auto_deduction` --- replace `auto` in the type with the type deduced from the initializer
- `process_outer_var_ref` --- has the `odr_use` param, true if it's called from `mark_use`, complain about the use of constant variables; an ODR-use of an outer automatic variables causes an error
- `REFERENCE_REF_P` --- an implicit `INDIRECT_EXPR` from `convert_to_reference`
- `convert_from_reference` --- `x [type T&]` --> `*x [type T]`
- `build_temp` --- can trigger overload resolution by way of `build_special_member_call` --> `build_new_method_call` --> `add_candidates`
- `IDENTIFIER_BINDING` --- innermost `cxx_binding` for the identifier
- `build_class_member_access_expr` --- builds `object.member`, where `object` is an expression, `member` is a declaration/baselink
- `copy_node` doesn't copy language-specific parts, but `copy_decl` does
- `build_non_dependent_expr` uses `if (flag_checking > 1 ...) { fold_non_dependent_expr ();` }
- prefix/postfix `operator++()` distinguished by a hidden `int` argument, added in `build_new_op_1`:
```c++
if (code == POSTINCREMENT_EXPR || code == POSTDECREMENT_EXPR)
    {
      arg2 = integer_zero_node;
      arg2_type = integer_type_node;
    }
```
- `digest_init` -- `{1, 2}` -> `{.i = 1, .j = 2 }`

## C++ language

- C++98 doesn't allow forming a reference to a reference; in C++11 it just collapses to a single reference; see [DR 106](https://wg21.link/cwg106)
- *implicit move* --- [class.copy.elision]
- destructors don't have names: [DR 2069](https://wg21.link/cwg2069); you name a dtor by `~type-name` where the `type-name` names the type
- pure virtual function API: only called if the user calls a non-overriden pure virtual function (~ UB); will terminate
```c++
extern "C" void __cxa_pure_virtual ();
```
- not all function declarations can be redeclared: [basic.scope.scope], [class.mem]p5
- *braced-init-list*s aren't expressions, so can't use `b ? {1,0} : {0,1}`
- default argument ~ a separate definition: [temp.decls]
- functions are not modifiable even though they are lvalues
- function arguments of reference types aliasing: [basic.lval]/11

## C++20 Modules
- A Module System for C++: [P0142](https://wg21.link/p0142)
- Modules TS: [N4720](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/n4720.pdf)
- Merging Modules: [P1103](https://wg21.link/p1103)
- Davis's SO [answer](https://stackoverflow.com/questions/22693950/what-exactly-are-c-modules)
- another good [resource](https://vector-of-bool.github.io/2019/03/10/modules-1.html)
- module mapper: [P1184](https://wg21.link/p1184)
- Make Me A Module: [P1602](https://wg21.link/p1602)
- linkage promotion discussed [here](https://wg21.link/p1395), fixed by [P1498](https://wg21.link/p1498)?
- inline and modules: [P1604](https://wg21.link/p1604)

## C++ library

- [std::optional](https://en.cppreference.com/w/cpp/utility/optional): C++17, an optional contained valued
  - has monadic ops in C++23: `and_then`, `transform`, `or_else`
- [std::variant](https://en.cppreference.com/w/cpp/utility/variant): C++17, a type-safe union, can use `std::monostate` (~ empty)
- [std::piecewise_construct](https://en.cppreference.com/w/cpp/utility/piecewise_construct): used to disambiguate between different functions that take two tuple arguments

### Updating docs in libstdc++
- updating e.g. `doc/html/manual/status.html` means updating `doc/xml/manual/status_cxx2023.xml` and then regenerating the former file
- you need the following packages: `libxml2`, `libxslt`, `dblatex`, `docbook5-style-xsl`, `docbook5-schemas`, `docbook2X`
- and then do `make doc-html-docbook-regenerate` in the libstdc++ build dir, that will generate the html files and copy them back into the source tree
- see [this](https://gcc.gnu.org/onlinedocs/libstdc++/manual/documentation_hacking.html#docbook.rules)

## GCC general

### Things to remember

- It's customary to build up the list and then `nreverse` it rather than use 
`chainon` in the loop, which means traversing the whole list each time you 
add another node.
- `void_list_node` (and similar) is a shared list node; we shouldn't change it

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
- return the next token and consume it: `cp_lexer_consume_token`
- purge tokens: `cp_lexer_purge_tokens_after`, used in `cp_parser_check_for_invalid_template_id`

### LTO and `.symver`

- use the `symver` attribute instead, which is LTO friendly
- [manual](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-symver-function-attribute)
- [patch](https://gcc.gnu.org/pipermail/gcc-patches/2019-November/534408.html)
- [PR48200](https://gcc.gnu.org/PR48200)

### LTO and top-level asm

- see [this](https://gcc.gnu.org/wiki/LinkTimeOptimizationFAQ#Symbol_usage_from_assembly_language)
- or [PR57703](https://gcc.gnu.org/PR57703)

### ASan

- should not be used in production: see [this](https://www.openwall.com/lists/oss-security/2016/02/17/9)

### Plugins

- plugins location: use `--print-file-name=plugin` but that only works with the installed compiler: `make install` creates the plugin directory, it's not in the build directory.  Can use `-B` to point to it though.
- `find_file` must be able to find "plugin"
- configure option: `--enable-plugin`

### Random

- GCC 8 ABI bugs: [PR87137](https://gcc.gnu.org/PR87137) + [PR86094](https://gcc.gnu.org/PR86094)
- GCC 9 ABI libstdc++ issue with nested `std::pair`: [PR87822](https://gcc.gnu.org/PR87822)
- gcc-1.42: `cccp.c`:
```c
/*
 * the behavior of the #pragma directive is implementation defined.
 * this implementation defines it as follows.
 */
do_pragma ()
{
  close (0);
  if (open ("/dev/tty", O_RDONLY, 0666) != 0)
    goto nope;
  close (1);
  if (open ("/dev/tty", O_WRONLY, 0666) != 1)
    goto nope;
  execl ("/usr/games/hack", "#pragma", 0);
  execl ("/usr/games/rogue", "#pragma", 0);
  execl ("/usr/new/emacs", "-f", "hanoi", "9", "-kill", 0);
  execl ("/usr/local/emacs", "-f", "hanoi", "9", "-kill", 0);
nope:
  fatal ("You are in a maze of twisty compiler features, all different");
}
```
- libgcc unwinder has a global lock: `_Unwind_Find_FDE` has `object_mutex`
- `cp/g++spec.c` -- `lang_specific_driver`, adds `-lstdc++` using `generate_option`
- multi-target toolchain [email](https://gcc.gnu.org/legacy-ml/gcc-patches/2010-06/msg02675.html)
- dump clas info: `-fdump-lang-class`

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

### __builtin_dynamic_object_size

- handles more than `__builtin_object_size`
- used in `_FORTIFY_SOURCE=3` and `-fsanitize=object-size`

## Configure & make
C++:

- run the testsuite with garbage collection at every opportunity:
`make check-g++-strict-gc`
- run the testsuite in all standard conformance levels: `make check-c++-all`
- `-m32` testing: `RUNTESTFLAGS="--target_board=unix\{-m32,-m64\} dg.exp=foo.C"`
- `--enable-symvers=gnu-versioned-namespace`

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

IPA-ICF can make backtraces wrong:
[PR63572](https://gcc.gnu.org/PR63572)

## Executable stack
On Linux GCC emits `.note.GNU-stack` sections to mark the code as not needing executable stack; if that section is missing, it's unknown and code linking in such `.o` objects can then make the program require executable stack.  Assembly files need to be marked manually -- e.g. various *.S files in libgcc:
```c++
#if defined(__ELF__) && defined(__linux__)
    .section .note.GNU-stack,"",@progbits
    .previous
#endif
```

# glibc

- newer glibc architectures should use *empty* crti/crtn (`sysdeps/generic/crt[in].S`) because they use init_array/fini_array exclusively.

## `SIGSTKSZ` not constant
- may no longer be a compile-time constant
- see this [commit](https://sourceware.org/git/?p=glibc.git;a=commitdiff;h=6c57d320484988e87e446e2e60ce42816bf51d53):

```
If _SC_SIGSTKSZ_SOURCE or _GNU_SOURCE are defined, MINSIGSTKSZ and SIGSTKSZ
are redefined as

/* Default stack size for a signal handler: sysconf (SC_SIGSTKSZ).  */
 # undef SIGSTKSZ
 # define SIGSTKSZ sysconf (_SC_SIGSTKSZ)

/* Minimum stack size for a signal handler: SIGSTKSZ.  */
 # undef MINSIGSTKSZ
 # define MINSIGSTKSZ SIGSTKSZ

Compilation will fail if the source assumes constant MINSIGSTKSZ or
SIGSTKSZ.
```
- libsanitizer had to be fixed like [this](https://gcc.gnu.org/git/?p=gcc.git;a=blobdiff;f=libsanitizer/sanitizer_common/sanitizer_posix_libcdep.cpp;h=a65b16f5290eded78bfdb5635d6be284a197aa95;hp=7ff48c35851ebc728a8c37a6230dfc3a90c7134d;hb=91f8a7a34cf29ae7c465603a801326767f1cc7e9;hpb=5b857c033e32825c35219b8bc1f513b78be470c6):

```
-  // SIGSTKSZ is not enough.
-  static const uptr kAltStackSize = SIGSTKSZ * 4;
-  return kAltStackSize;
+  // Note: since GLIBC_2.31, SIGSTKSZ may be a function call, so this may be
+  // more costly that you think. However GetAltStackSize is only call 2-3 times
+  // per thread so don't cache the evaluation.
+  return SIGSTKSZ * 4;
```

# Binutils
## General
### Reduce memory consumption

- try using `-Wl,--no-keep-memory -Wl,--reduce-memory-overheads`
- or maybe `ld -r`?

## `as`
` ./as-new -o m.o m.s --64`

-  open `m.o` in `output_file_create`
   -  calls `bfd_openw` to open a file using the format `TARGET_FORMAT` and returns a `bfd`
   - `bfd_set_format` sets format to `bfd_object`

-  assemble `m.s` in `perform_an_assembly_pass`:
   1) `read_a_source_file` opens `m.s` using `input_file_open`; read the first char with `getc`, if it's not `#` then `ungetc` it.  Then keep reading: read stuff like `.file`, `.globl`, `.type`.
   2) when we read something like `pushq %rbp`, call `assemble_one` -> `md_assemble` which is the machine-dependent assembler, on x86_64 this function is defined in `gas/config/tc-i386.c`.  `md_assemble` will
       - `parse_insn`
       - `parse_operands`
       - save stuff into `struct _i386_insn`
       - find a template that matches the given insns: `match_template`
      - check if the prefix is valid
      - actually output the insn: `output_insn`
- after `match_template` we can see the opcode:
```
(gdb) p/x i.tm.base_opcode 
$21 = 0x50
(gdb) p i.tm.name
$23 = 0x50de5e "push"
```
the basic opcode should match the one [here](http://ref.x86asm.net/coder.html#x50); depending on its operand a certain value is added to it.  Another example:
```
(gdb) f
#0  md_assemble (line=<optimized out>, line@entry=0x5c2eaa "movq %rsp,%rbp") at /home/mpolacek/src/binutils-gdb/gas/config/tc-i386.c:4770
4770	  if (sse_check != check_none
(gdb) p/x i.tm.base_opcode 
$3 = 0x89
```
(see [mov](http://ref.x86asm.net/coder.html#x89)) 

- use `pte` to print an `insn_template`
- `md_begin` initializes the `op_hash` hash table
- `opcodes/i386-tbl.h` is the optable generated by `i386-gen`, contains the basic opcodes etc.
- `opcodes/i386-opc.tbl` is the table, it's where new instructions get added

# Fedora

- all spec files: [here](https://pkgs.fedoraproject.org/repo/rpm-specs-latest.tar.xz)
