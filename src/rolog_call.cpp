#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"

using namespace Rcpp;

extern PlEngine* rolog_engine ;

// [[Rcpp::export]]
CharacterVector rolog_call(String prolog_call) 
{
  CharacterVector r ;
  if(!rolog_engine)
  {
    r.push_back("Rolog not initialized") ;
    return r ;
  } // if

  PlCall(prolog_call.get_cstring()) ;
  return 0 ;
} // rolog_call
