#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"

using namespace Rcpp;

extern PlEngine* rolog_engine ;

// [[Rcpp::export]]
CharacterVector rolog_consult(CharacterVector prolog_files) 
{
  CharacterVector r ;
  if(!rolog_engine)
  {
    r.push_back("Rolog not initialized") ;
    return r ;
  } // if

  for(R_xlen_t i=0 ; i<prolog_files.length() ; i++)
  {
    PlQuery q("consult", PlString(prolog_files[i])) ;
    if(!q.next_solution())
      continue ;
    
    r.push_back(prolog_files[i]) ;
  } // for

  return r ;
} // rolog_consult
