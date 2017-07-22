#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"
#include <iostream>

using namespace Rcpp;

SEXP rolog_get(PlTerm t)
{
  if(PL_is_integer(t))
    return(IntegerVector::create(t)) ;
  
  if(PL_is_number(t))
    return(NumericVector::create(t)) ;
  
  if(PL_is_string(t))
  {
    CharacterVector v(1) ;
    v[0] = (wchar_t*) t ;
    return(v) ;
  }
  
  if(PL_is_atom(t))
  {
    CharacterVector v(1) ;
    v[0] = (wchar_t*) t ;
    v.names() = "Atom" ;
    return(v) ;
  }
  
  if(PL_is_list(t))
  {
    List l ;
    term_t head = PL_new_term_ref() ;   
    term_t list = PL_copy_term_ref(t) ; 
    while(PL_get_list(list, head, list))
      l.push_back(rolog_get(head)) ;

    return(l) ;
  }
  
  if(PL_is_compound(t))
  {
    List l ;
    l.push_back(t.name()) ;
    l.names() = "Function" ;
    for(int i=1 ; i<=t.arity() ; i++)
      l.push_back(rolog_get(t[i])) ;
    return(l) ;
  }
  
  return(NumericVector(0)) ;
}
  
// [[Rcpp::export]]
List rolog_query_LL(String predicate, StringVector arguments) 
{
  PlTermv arg(2) ;
  arg[0] = PlCompound(String(arguments[0]).get_cstring()) ;
  PlQuery q(predicate.get_cstring(), arg) ;
    
  List r ;
  while(q.next_solution())
    r.push_back(rolog_get(arg[1])) ;

  return r ;
} // rolog_query_LL
