#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"
#include <iostream>

using namespace Rcpp;

SEXP leaf(SEXP l) ;

PlCompound leaf_lang(SEXP l)
{
  int i=0 ;
  for(SEXP cons=CDR(l) ; cons != R_NilValue ; cons = CDR(cons))
    i++ ;
  
  PlTermv v(i) ;
  for(SEXP cons=CDR(l) ; cons != R_NilValue ; cons = CDR(cons))
    v[i++] = leaf(CAR(cons)) ;

  Symbol pred = as<Symbol>(CAR(l)) ;
  PlCompound c(pred.c_str(), v) ;
  return c ;
}

PlTerm leaf_na(SEXP l)
{
  return PlTerm((long) -1) ;
}

PlTerm leaf_real(SEXP l)
{
  NumericVector v(1) ;
  v[0] = as<double>(l) ;
  return PlTerm(v[0]) ;
}

PlTerm leaf_int(IntegerVector l)
{
  IntegerVector v(1) ;
  v[0] = -l[0] ;
  return PlTerm((double) v[0]) ;
}

PlTerm leaf_symbol(SEXP l)
{
  Symbol v = as<Symbol>(l) ;
  return PlAtom(v.c_str()) ;
}

PlTerm leaf_string(StringVector l)
{
  StringVector v(1) ;
  v[0] = l[0] ;
  return PlString(v[0].get_cstring()) ;
}

PlTerm leaf(SEXP l)
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
List rolog_query(String predicate, SEXP call) 
{
  PlTerm v = leaf(call) ;
  return List(1) ;

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
