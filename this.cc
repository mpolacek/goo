// This and ref-qualifiers.

struct S {
  void f1();	  // #1
  void f2() &&;	  // #2
  void f3() &;	  // #3
};

/* There's a difference between #1 and #3: old special handling permits
   an rvalue to be bound to an lvalue reference to non-const type when
   it's *this.  See build_this_conversion:  */
#if 0
  bool this_p = true;
  if (FUNCTION_REF_QUALIFIED (TREE_TYPE (fn)))
    {
      /* If the function has a ref-qualifier, the implicit
         object parameter has reference type.  */
      bool rv = FUNCTION_RVALUE_QUALIFIED (TREE_TYPE (fn));
      parmtype = cp_build_reference_type (parmtype, rv);
      /* The special handling of 'this' conversions in compare_ics
         does not apply if there is a ref-qualifier.  */
      this_p = false;
#endif

int
main ()
{
  S().f1();
  S().f2();
  //S().f3(); // error (build_over_call)
}
