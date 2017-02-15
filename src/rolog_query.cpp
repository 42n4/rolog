#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"

using namespace Rcpp;

// [[Rcpp::export]]
CharacterVector rolog_query(CharacterVector predicate, CharacterVector arguments) 
{
  PlTermv arg(2) ;
  arg[0] = PlCompound((char*) arguments[0]) ;
  PlQuery q((char*) predicate[0], arg) ;

  CharacterVector r ;
  while(q.next_solution())
    r.push_back((char*) arg[1]) ;
  return r ;
} // rolog_query
