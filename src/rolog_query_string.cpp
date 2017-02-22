#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"

using namespace Rcpp;

// [[Rcpp::export]]
List rolog_query_string(String predicate, StringVector arguments) 
{
  PlFrame f ;
  PlTermv arg(2) ;
  arg[0] = PlString(String(arguments[0]).get_cstring()) ;
  // PlQuery q(predicate.get_cstring(), arg) ;
  
  List r ;
  if(PlCall(predicate.get_cstring(), arg))
    r.push_back(String((char*) (arg[1]))) ;

  // while(q.next_solution())
    // r.push_back(String((char*) arg[1])) ;
  return r ;
} // rolog_query_string
