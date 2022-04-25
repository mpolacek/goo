/* Decide a CFL in polynomial time (in O(n^3)).  */

#include <string>
#include <vector>

using rules_t = std::vector<std::string>;

/* Grammar.  */
class grammar {
  char start_var;
  rules_t rules;
public:
  char get_start_var () const { return start_var; }
};

/* Chomsky normal form grammar.  */
class grammar_cnf : grammar {
public:
  grammar_cnf () : grammar{} { }
  bool valid_p () const;
};

/* Verify that a grammar is indeed in CNF.  */

bool
grammar_cnf::valid_p () const
{
  auto g = *this;
  return true;
}

int
main ()
{
  // a grammar should be initialized with rules,
  // and we parse them to get vars, terminals, start var
  grammar_cnf cnf;
  if (!cnf.valid_p ())
    return 1;
}
