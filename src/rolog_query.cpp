#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"
#include <iostream>

using namespace Rcpp;

PlTerm leaf(SEXP l) ;

PlCompound leaf_lang(SEXP l)
{
  cout << "entering PLCompound" << endl ;
  int i=0 ;
  for(SEXP cons=CDR(l) ; cons != R_NilValue ; cons = CDR(cons))
    i++ ;
  cout << i << " arguments" << endl ;
  
  PlTermv v(i) ;
  for(SEXP cons=CDR(l) ; cons != R_NilValue ; cons = CDR(cons))
    v[i++] = leaf(CAR(cons)) ;
  cout << i << " arguments instantiated" << endl ;

  Symbol pred = as<Symbol>(CAR(l)) ;
  cout << pred.c_str() << "/" << i << " initialized" << endl ;
  
  PlCompound c(pred.c_str(), v) ;
  cout << "leaving PLCompound" << endl ;
  return c ;
}

PlTerm leaf_na(SEXP l)
{
  cout << "leaf_na" << endl ;
  return PlTerm((long) -1) ;
}

PlTerm leaf_real(SEXP l)
{
  cout << "leaf_real" << endl ;
  NumericVector v(1) ;
  v[0] = as<double>(l) ;
  return PlTerm(v[0]) ;
}

PlTerm leaf_int(IntegerVector l)
{
  cout << "leaf_int" << endl ;
  IntegerVector v(1) ;
  v[0] = -l[0] ;
  return PlTerm((double) v[0]) ;
}

PlTerm leaf_symbol(SEXP l)
{
  cout << "leaf_symbol" << endl ;
  Symbol v = as<Symbol>(l) ;
  return PlAtom(v.c_str()) ;
}

PlTerm leaf_string(StringVector l)
{
  cout << "leaf_string" << endl ;
  StringVector v(1) ;
  v[0] = l[0] ;
  return PlString(String(v[0]).get_cstring()) ;
}

PlTerm leaf(SEXP l)
{
  cout << "leaf dispatcher" << endl ;
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
  cout << "rolog_query" << endl ;
  PlTerm v = leaf(call) ;
  cout << "rolog_query done" << endl ;
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
