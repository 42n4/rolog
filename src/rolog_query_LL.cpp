#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"
#include <iostream>

using namespace Rcpp;

// [[Rcpp::export]]
List rolog_query_LL(String predicate, StringVector arguments) 
{
  PlTermv arg(arguments.length()) ;
  for(int i=0 ; i<arguments.length() ; i++)
    if(String(arguments[i]) != String(""))
      arg[i] = PlCompound(String(arguments[i]).get_cstring()) ;
  PlQuery q(predicate.get_cstring(), arg) ;

  List r ;
  while(q.next_solution())
    for(int i=0 ; i<arguments.length() ; i++)
      if(String(arguments[i]) == String(""))
        r.push_back(String((wchar_t*) arg[i])) ;
  return r ;
} // rolog_query_LL
