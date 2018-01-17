#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"
#include <iostream>

using namespace Rcpp;

PlTerm leaf(SEXP l) ;
SEXP pl2r_leaf(PlTerm t) ;

// translate R expression to Prolog compound
PlTerm leaf_lang(SEXP l)
{
  // Count number of arguments
  int i = 0 ;
  for(SEXP cons=CDR(l) ; cons != R_NilValue ; cons = CDR(cons))
    i++ ;
  
  // Collect arguments
  PlTermv v(i) ;
  i = 0 ;
  for(SEXP cons=CDR(l) ; cons != R_NilValue ; cons = CDR(cons))
  {
    if(TAG(cons) == R_NilValue)
    {
      v[i++] = PlTerm(leaf(CAR(cons))) ;
      continue ;
    }
    
    Symbol argname = as<Symbol>(TAG(cons)) ;
    v[i++] = PlCompound("=", PlTermv(PlAtom(argname.c_str()), PlTerm(leaf(CAR(cons))))) ;
  }
  
  // Construct term
  Symbol pred = as<Symbol>(CAR(l)) ;
  PlCompound c(pred.c_str(), v) ;
  if(i > 0)
    return(PlCompound(pred.c_str(), v)) ;
  
  return(PlCompound(pred.c_str(), (size_t) 0)) ;
}

// translate Prolog compound to R expression
SEXP pl2r_lang(PlTerm t)
{
  Pairlist l ;
  StringVector v(1) ;
  v[0] = t.name() ;
  Symbol pred = as<Symbol>(v) ;
  l.push_back(pred) ;
  for(int i=1 ; i<=t.arity() ; i++)
  {
    // compounds like '='(x, y) are named arguments
    if(PL_is_compound(t[i]) && t[i].name() == std::string("=") && t[i].arity() == 2)
    {
      PlTerm u = t[i] ;
      l.push_back(Named(u[1].name()) = pl2r_leaf(u[2])) ;
      continue ;
    }
      
    // other arguments
    SEXP leaf = pl2r_leaf(t[i]) ;
    l.push_back(leaf) ;
  }
  
  SEXP s = l ;
  SET_TYPEOF(s, LANGSXP) ;
  return(s) ;
}

// translate Prolog list to R list
SEXP pl2r_list(PlTerm t)
{
  List l ;
  PlTail tail(t) ;
  PlTerm e ;
  while(tail.next(e))
    l.push_back(pl2r_leaf(e)) ;
  return l ;
}

// Prolog NA
PlTerm leaf_na(SEXP l)
{
  return PlTerm((long) -1) ;
}

// R NA
SEXP leaf_na(PlTerm l)
{
  NumericVector v(1) ;
  v[0] = -1 ;
  return v ;
}

// real number
PlTerm r2pl_real(SEXP l)
{
  NumericVector v = as<NumericVector>(l) ;
  if(v.size() == 1)
    return(PlTerm((double) v[0])) ;
  
  PlTerm list ;
  PlTail t(list) ;
  for(int i=0 ; i<v.size() ; i++)
    t.append(PlTerm((double) v[i])) ;
  t.close() ;
  return list ;
}

// real number
SEXP leaf_real(PlTerm l)
{
  NumericVector v(1) ;
  v[0] = (double) l ;
  return v ;
}

// integer
PlTerm leaf_int(IntegerVector l)
{
  IntegerVector v(1) ;
  v[0] = l[0] ;
  return PlTerm((long) (v[0])) ;
}

SEXP pl2r_int(PlTerm l)
{
  IntegerVector v(1) ;
  v[0] = (int) l ;
  return v ;
}

PlTerm leaf_symbol(SEXP l)
{
  Symbol v = as<Symbol>(l) ;
  return PlAtom(v.c_str()) ;
}

PlTerm leaf_logical(SEXP l)
{
  LogicalVector v = as<LogicalVector>(l) ;
  if(v[0] == TRUE)
    return PlAtom("TRUE") ;
  if(v[0] == FALSE)
    return PlAtom("FALSE") ;

  return leaf_na(l) ;
}

SEXP leaf_symbol(PlTerm l)
{
  Symbol s = Symbol((char*) l) ;
  LogicalVector v(1) ;
  v[0] = TRUE ;
  if(s == "TRUE")
    return v ;
  v[0] = FALSE ;
  if(s == "FALSE")
    return v ;
  return s ;  
}

// String
PlTerm r2pl_string(CharacterVector l)
{
  if(l.size() == 1)
    return PlTerm(String(l[0]).get_cstring()) ;
  
  PlTerm list ;
  PlTail t(list) ;
  for(int i=0 ; i<l.size() ; i++)
    t.append(PlTerm(String(l[i]).get_cstring())) ;
  t.close() ;
  return list ;
}

SEXP pl2r_string(PlTerm l)
{
  StringVector v(1) ;
  v[0] = (wchar_t*) l ;
  return v ;
}

PlTerm leaf(SEXP l)
{
  if(TYPEOF(l) == LANGSXP)
    return leaf_lang(l) ;
  
  if(TYPEOF(l) == REALSXP)
    return r2pl_real(l) ;

  if(TYPEOF(l) == LGLSXP)
    return leaf_logical(l) ;

  if(TYPEOF(l) == INTSXP)
    return leaf_int(l) ;

  if(TYPEOF(l) == SYMSXP)
    return leaf_symbol(l) ;

  if(TYPEOF(l) == STRSXP)
    return r2pl_string(l) ;
  
  return leaf_na(l) ;
}

SEXP pl2r_leaf(PlTerm t)
{
  if(PL_is_integer(t))
    return(pl2r_int(t)) ;

  if(PL_is_number(t))
    return(leaf_real(t)) ;
  
  if(PL_is_string(t))
    return pl2r_string(t) ;
  
  if(PL_is_atom(t))
    return leaf_symbol(t) ;
  
  if(PL_is_list(t))
    return pl2r_list(t) ;

  if(PL_is_compound(t))
    return pl2r_lang(t) ;

  return leaf_na(t) ;
}

// [[Rcpp::export]]
List rolog_query(String predicate, SEXP call) 
{
  PlTermv arg(2) ;
  arg[0] = leaf(call) ;
  PlQuery q(predicate.get_cstring(), arg) ;

  List r ;
  while(q.next_solution())
    r.push_back(pl2r_leaf(arg[1])) ;
  return r ;
} // rolog_query
