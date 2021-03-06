#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"
#include <iostream>

using namespace Rcpp;

// [[Rcpp::export]]
List rolog_query_comp(String predicate, StringVector arguments) 
{
  PlTermv arg(2) ;
  arg[0] = PlCompound(String(arguments[0]).get_cstring()) ;
  PlQuery q(predicate.get_cstring(), arg) ;

  List r ;
  while(q.next_solution())
    r.push_back(String((wchar_t*) arg[1])) ;
  return r ;
} // rolog_query_comp
