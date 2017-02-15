#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"

using namespace Rcpp;

// [[Rcpp::export]]
CharacterVector rolog_consult(CharacterVector prolog_files) 
{
  CharacterVector r ;
  for(R_xlen_t i=0 ; i<prolog_files.length() ; i++)
  {
    PlQuery q("consult", PlString(prolog_files[i])) ;
    if(!q.next_solution())
      continue ;
    
    r.push_back(prolog_files[i]) ;
  } // for

  return r ;
} // rolog_consult
