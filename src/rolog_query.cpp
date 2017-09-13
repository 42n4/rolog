#include <Rcpp.h>
#include <Rcpp/PairList.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"
#include <iostream>

using namespace Rcpp;

PlTerm leaf(SEXP l) ;

// translate R expression to Prolog compound
PlCompound leaf_lang(SEXP l)
{
  // Count number of arguments
  int i = 0 ;
  for(SEXP cons=CDR(l) ; cons != R_NilValue ; cons = CDR(cons))
    i++ ;
  
  // Collect arguments
  PlTermv v(i) ;
  i = 0 ;
  for(SEXP cons=CDR(l) ; cons != R_NilValue ; cons = CDR(cons))
    v[i++] = PlTerm(leaf(CAR(cons))) ;

  // Construct term
  Symbol pred = as<Symbol>(CAR(l)) ;
  return PlCompound(pred.c_str(), v) ;
}

// translate Prolog compound to R expression
SEXP leaf_lang(PlCompound t)
{
  Rcpp::PairList l ;
  l.push_back(t.name()) ;
  for(int i=1 ; i<=t.arity() ; i++)
    l.push_back(leaf(t[i])) ;
  return(l) ;
}

// Prolog NA
PlTerm leaf_na(SEXP l)
{
  return PlTerm((long) -1) ;
}

// R NA
SEXP leaf_na(PlTerm l)
{
  return PlTerm(NumericVector::create(-1)) ;
}

// real number
PlTerm leaf_real(SEXP l)
{
  NumericVector v(1) ;
  v[0] = as<double>(l) ;
  return PlTerm((double) (v[0])) ;
}

// real number
SEXP leaf_na(PlTerm l)
{
  return PlTerm(NumericVector::create(l)) ;
}

// integer
PlTerm leaf_int(IntegerVector l)
{
  IntegerVector v(1) ;
  v[0] = l[0] ;
  return PlTerm((double) v[0]) ;
}

SEXP leaf_int(PlTerm l)
{
  return PlTerm(IntegerVector::create(l)) ;
}

PlTerm leaf_symbol(SEXP l)
{
  Symbol v = as<Symbol>(l) ;
  return PlAtom(v.c_str()) ;
}

SEXP leaf_symbol(PLAtom l)
{
  Symbol v = as<Symbol>(l) ;
  return v ;
}

PlTerm leaf_string(StringVector l)
{
  StringVector v(1) ;
  v[0] = l[0] ;
  return PlString(String(v[0]).get_cstring()) ;
}

SEXP leaf_string(PLString l)
{
  StringVector v(1) ;
  v[0] = (wchat_t*) l ;
  return v ;
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

SEXP leaf(PlTerm t)
{
  if(PL_is_integer(t))
    return(leaf_int(t)) ;

  if(PL_is_number(t))
    return(leaf_real(t)) ;
  
  if(PL_is_string(t))
    return leaf_string(t) ;
  
  if(PL_is_atom(t))
    return leaf_symbol(t) ;
  
  if(PL_is_compound(t))
    return leaf_lang(t) ;

  return leaf_na(l) ;
}

// [[Rcpp::export]]
List rolog_query(String predicate, SEXP call) 
{
  PlTermv arg(2) ;
  arg[0] = leaf(call) ;
  PlQuery q(predicate.get_cstring(), arg) ;

  List r ;
  while(q.next_solution())
    r.push_back(String((wchar_t*) arg[1])) ;
  return r ;
} // rolog_query
