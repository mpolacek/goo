// The digraph hack.
// Valid C++98, invalid C++11.
// libcpp/lex.c:_cpp_lex_direct:
#if 0
      else if (CPP_OPTION (pfile, digraphs))
	{
	  if (*buffer->cur == ':')
	    {
	      /* C++11 [2.5/3 lex.pptoken], "Otherwise, if the next
		 three characters are <:: and the subsequent character
		 is neither : nor >, the < is treated as a preprocessor
		 token by itself".  */
	      if (CPP_OPTION (pfile, cplusplus)
		  && CPP_OPTION (pfile, lang) != CLK_CXX98
		  && CPP_OPTION (pfile, lang) != CLK_GNUCXX
		  && buffer->cur[1] == ':'
		  && buffer->cur[2] != ':' && buffer->cur[2] != '>')
		break;

	      buffer->cur++;
	      result->flags |= DIGRAPH;
	      result->type = CPP_OPEN_SQUARE;
	    }
#endif

#define F(X) X ## :
int a[] = { 1, 2, 3}, i = 1;
int n = a F(<::)i];
