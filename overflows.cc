// Checking for overflows.

#include <climits>
#include <limits>

#define assert(X) do { if (!(X)) __builtin_abort (); } while(0)

template<typename T>
bool ovf1 (T lhs, T rhs)
{
  auto min = std::numeric_limits<T>::min ();
  auto max = std::numeric_limits<T>::max ();
  return (rhs < 0
	  ? (lhs < min - rhs)
	  : (lhs > max - rhs));
}
/* GCC -O2 generates:
	testl	%esi, %esi
	js	.L4
	movl	$2147483647, %eax
	subl	%esi, %eax
	cmpl	%edi, %eax
	setl	%al
	ret
.L4:
	movl	$-2147483648, %eax
	subl	%esi, %eax
	cmpl	%edi, %eax
	setg	%al
	ret
 */

template<typename T>
bool ovf2 (T lhs, T rhs)
{
  T tmp;
  return __builtin_add_overflow (lhs, rhs, &tmp);
}

/* GCC -O2 generates:
	addl	%esi, %edi
	seto	%al
	ret
 */

// Explicit instantiations to get the asm.
template bool ovf1<int>(int, int);
template bool ovf2<int>(int, int);

int
main ()
{
  assert (ovf1 (INT_MAX, 1));
  assert (ovf2 (INT_MAX, 1));
  assert (ovf1 (INT_MIN, -1));
  assert (ovf2 (INT_MIN, -1));
  assert (!ovf1 (unsigned(INT_MAX), 1u));
  assert (!ovf2 (unsigned(INT_MAX), 1u));
}
