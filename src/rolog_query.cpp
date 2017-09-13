#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"
#include <iostream>

using namespace Rcpp;

SEXP leaf(SEXP l) ;

SEXP leaf_lang(SEXP l)
{
  List lang(1) ;
  lang[0] = as<Symbol>(CAR(l)) ;
  
  for(SEXP cons=CDR(l) ; cons != R_NilValue ; cons = CDR(cons))
    lang.push_back(leaf(CAR(cons))) ;
  
  return lang ;
}

SEXP leaf_na(SEXP l)
{
  NumericVector v(1) ;
  v[0] = -1 ;
  return v ;
}

SEXP leaf_real(SEXP l)
{
  NumericVector v(1) ;
  v[0] = as<double>(l) ;
  return v ;
}

SEXP leaf_int(IntegerVector l)
{
  IntegerVector v(1) ;
  v[0] = -l[0] ;
  return v ;
}

SEXP leaf_symbol(SEXP l)
{
  Symbol v = as<Symbol>(l) ;
  return v ;
}

SEXP leaf_string(StringVector l)
{
  StringVector v(1) ;
  v[0] = l[0] ;
  return v ;
}

SEXP leaf(SEXP l)
{
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
List rolog_query(String predicate, SEXP arguments) 
{
  return leaf(arguments) ;

  /*
  PlTermv arg(2) ;
  arg[0] = PlCompound(String(arguments[0]).get_cstring()) ;
  PlQuery q(predicate.get_cstring(), arg) ;

  List r ;
  while(q.next_solution())
    r.push_back(String((wchar_t*) arg[1])) ;
  return r ;
  */
} // rolog_query
