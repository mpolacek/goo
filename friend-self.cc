#if 0
See make_friend_class:

  /* It makes sense for a template class to be friends with itself,
     that means the instantiations can be friendly.  Other cases are
     not so meaningful.  */
  if (!friend_depth && same_type_p (type, friend_type))
    {
      if (complain)
        warning (0, "class %qT is implicitly friends with itself",
                 type);
      return;
    }

#endif

template<typename T>
class S {
  // Without this, s.foo below won't be accessible.
  template<typename> friend class S;
  void foo ();
};

template<> class S<int> {
  void bar () { S<char> s; s.foo (); }
};
