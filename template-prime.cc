// Compute prime_p at compile time using templates.

template<unsigned int P, unsigned int D>
struct do_prime {
  static constexpr bool value = (P % D) && do_prime<P, D - 1>::value;
};

template<unsigned int P>
struct do_prime<P, 2> {
  static constexpr bool value = (P % 2 != 0);
};

template<unsigned int P>
struct prime_p {
  static constexpr bool value = do_prime<P, P / 2>::value;
};

template<>
struct prime_p<0> { static constexpr bool value = false; };
template<>
struct prime_p<1> { static constexpr bool value = false; };
template<>
struct prime_p<2> { static constexpr bool value = true; };
template<>
struct prime_p<3> { static constexpr bool value = true; };

static_assert(!prime_p<1>::value);
static_assert(prime_p<2>::value);
static_assert(prime_p<3>::value);
static_assert(!prime_p<4>::value);
static_assert(prime_p<5>::value);
static_assert(!prime_p<6>::value);
static_assert(prime_p<7>::value);
static_assert(!prime_p<8>::value);
static_assert(!prime_p<9>::value);
static_assert(!prime_p<10>::value);
static_assert(prime_p<11>::value);
static_assert(!prime_p<12>::value);
