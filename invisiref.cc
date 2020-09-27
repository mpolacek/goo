// Pass by invisible reference.

void *as[5];
int i;

// Since
// (type_has_nontrivial_copy_init (t)
//  || TYPE_HAS_NONTRIVIAL_DESTRUCTOR (t))
// holds, this class will be passed by invisible reference.
// See <https://itanium-cxx-abi.github.io/cxx-abi/abi.html#value-parameter>.

struct A {
  A() {
    __builtin_printf ("A()\n");
    as[i++] = this;
  }
  A(const A& a) {
    __builtin_printf ("A(const A&)\n");
    if (&a != as[i - 1])
     __builtin_abort ();
    as[i++] = this;
  }
  ~A() {
    __builtin_printf ("~A()\n");
    if (this != as[--i])
      __builtin_abort ();
  }
  char arr[16];
};

void
foo (A)
{
}

void
doit ()
{
  A a;
  foo (a);
}

struct B {
  int i;
  ~B() = default; // trivial
};

/* If the class is INTEGER, the next available register of the sequence
   %rax, %rdx is used.

	pushq	%rbp
	movq	%rsp, %rbp

	movl	$0, %eax

	popq	%rbp
	ret
 */
B retb () { return B(); }

struct C {
  int i;
  ~C() { }  // non-trivial
};

/* If the type has class MEMORY, then the caller provides space for the return
   value and passes the address of this storage in %rdi as if it were the first
   argument to the function.  In effect, this address becomes a “hidden” first
   argument.  On return %rax will contain the address that has been passed in by
   the caller in %rdi.

	pushq	%rbp
	movq	%rsp, %rbp

	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movl	$0, (%rax)
	movq	-8(%rbp), %rax

	popq	%rbp
	ret
 */
C retc () { return C(); }

int
main ()
{
  doit ();
  if (i != 0)
    __builtin_abort ();
}
