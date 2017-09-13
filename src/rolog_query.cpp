#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"
#include <iostream>

using namespace Rcpp;

PlTerm leaf(SEXP l) ;

PlCompound leaf_lang(SEXP l)
{
  std::cout << "entering PLCompound" << std::endl ;
  int i = 0 ;
  for(SEXP cons=CDR(l) ; cons != R_NilValue ; cons = CDR(cons))
    i++ ;
  std::cout << i << " arguments" << std::endl ;
  
  PlTermv v(i) ;
  i = 0 ;
  for(SEXP cons=CDR(l) ; cons != R_NilValue ; cons = CDR(cons))
  {
    std::cout << "argument " << i << std::endl ;
    v[i++] = PlTerm(leaf(CAR(cons))) ;
    std::cout << "argument " << i-1 << "done" << std::endl ;
  }
  std::cout << i << " arguments instantiated" << std::endl ;

  Symbol pred = as<Symbol>(CAR(l)) ;
  std::cout << pred.c_str() << "/" << i << " initialized" << std::endl ;
  
  PlCompound c(pred.c_str(), v) ;
  std::cout << "leaving PLCompound" << std::endl ;
  return c ;
}

PlTerm leaf_na(SEXP l)
{
  std::cout << "leaf_na" << std::endl ;
  return PlTerm((long) -1) ;
}

PlTerm leaf_real(SEXP l)
{
  std::cout << "leaf_real" << std::endl ;
  NumericVector v(1) ;
  v[0] = as<double>(l) ;
  std::cout << "leaf_real" << (double) (v[0]) << std::endl ;
  return PlTerm((double) (v[0])) ;
}

PlTerm leaf_int(IntegerVector l)
{
  std::cout << "leaf_int" << std::endl ;
  IntegerVector v(1) ;
  v[0] = -l[0] ;
  return PlTerm((double) v[0]) ;
}

PlTerm leaf_symbol(SEXP l)
{
  std::cout << "leaf_symbol" << std::endl ;
  Symbol v = as<Symbol>(l) ;
  return PlAtom(v.c_str()) ;
}

PlTerm leaf_string(StringVector l)
{
  std::cout << "leaf_string" << std::endl ;
  StringVector v(1) ;
  v[0] = l[0] ;
  return PlString(String(v[0]).get_cstring()) ;
}

PlTerm leaf(SEXP l)
{
  std::cout << "leaf dispatcher" << std::endl ;
  if(TYPEOF(l) == LANGSXP)
    return leaf_lang(l) ;
  
  if(TYPEOF(l) == REALSXP)
    return leaf_real(l) ;

  if(TYPEOF(l) == INTSXP)
    return leaf_int(l) ;

  if(TYPEOF(l) == SYMSXP)
    return leaf_symbol(l) ;

  if(TYPEOF(l) == STRSXP)
    return leaf_string(l) ;
  
  return leaf_na(l) ;
}

// [[Rcpp::export]]
List rolog_query(String predicate, SEXP call) 
{
  std::cout << "rolog_query" << std::endl ;
  PlTerm v = leaf(call) ;
  std::cout << "rolog_query done" << std::endl ;

  PlTermv arg(2) ;
  arg[0] = v
  PlQuery q(predicate.get_cstring(), arg) ;

  List r ;
  while(q.next_solution())
    r.push_back(String((wchar_t*) arg[1])) ;
  return r ;
} // rolog_query
