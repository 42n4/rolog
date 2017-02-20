// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <Rcpp.h>

using namespace Rcpp;

// rcpp_hello_world
List rcpp_hello_world();
RcppExport SEXP rolog_rcpp_hello_world() 
{
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    rcpp_result_gen = Rcpp::wrap(rcpp_hello_world());
    return rcpp_result_gen;
END_RCPP
}

// rolog_exit
void rolog_exit();
RcppExport void rolog_rolog_exit() 
{
  Rcpp::RNGScope rcpp_rngScope_gen;
  rolog_exit();
}

// rolog_init
int rolog_init(String home, String prc);
RcppExport SEXP rolog_rolog_init(SEXP home, SEXP prc) 
{
  BEGIN_RCPP
  Rcpp::RObject rcpp_result_gen;
  Rcpp::RNGScope rcpp_rngScope_gen;
  rcpp_result_gen = Rcpp::wrap(rolog_init(home, prc));
  return rcpp_result_gen;
  END_RCPP
}

// rcpp_consult
CharacterVector rolog_consult(CharacterVector prolog_files);
RcppExport SEXP rolog_rolog_consult(SEXP prolog_files) 
{
  BEGIN_RCPP
  Rcpp::RObject rcpp_result_gen;
  Rcpp::RNGScope rcpp_rngScope_gen;
  rcpp_result_gen = Rcpp::wrap(rolog_consult(prolog_files));
  return rcpp_result_gen;
  END_RCPP
}

// rolog_query
List rolog_query(String predicate, StringVector arguments);
RcppExport SEXP rolog_rolog_query(SEXP predicate, SEXP arguments) 
{
  BEGIN_RCPP
  Rcpp::RObject rcpp_result_gen;
  Rcpp::RNGScope rcpp_rngScope_gen;
  rcpp_result_gen = Rcpp::wrap(rolog_query(predicate, arguments));
  return rcpp_result_gen;
  END_RCPP
}
