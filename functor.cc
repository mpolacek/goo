// Functor.

struct S {
  S() : calls(0) { }
  int operator()() const {
    return ++calls;
  }
  mutable int calls;
};

int
main ()
{
  S s;
  s();
#if 0
  - finish_call_expr will see that fn is a VAR_DECL of type S:

    else if (CLASS_TYPE_P (TREE_TYPE (fn)))
      /* If the "function" is really an object of class type, it might
	 have an overloaded `operator ()'.  */
      result = build_op_call (fn, args, complain);

  - build_op_call_1 builds a new call to operator():
    - look for operator() in S:

        fns = lookup_fnfields (TYPE_BINFO (type), call_op_identifier, 1, complain);

      which finds a BASELINK "int S::operator()() const"

    - call resolve_args -- does nothing here
    - add what we found to candidates:

        add_candidates (BASELINK_FUNCTIONS (fns), ...)

      BASELINK_FUNCTIONS (fns) is a FUNCTION_DECL operator()
    - now
        convs = lookup_conversions (type);
      tries to get a list of all the non-hidden user-defined conversion
      functions for S, but there are none
    - call
	candidates = splice_viable (candidates, true, &any_viable_p);
      any_viable_p is true; we only have one candidate: FUNCTION_DECL operator()
      so

	cand = tourney (candidates, complain);

      selects that one.  cand->fn is a FUNCTION_DECL so we do

	result = build_over_call (cand, LOOKUP_NORMAL, complain);

      and that creates

	S::operator() (&s)

      which is what finish_call_expr returns.
#endif
  s();
  s();
  if (s.calls != 3)
    __builtin_abort ();
}

