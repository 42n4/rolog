#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"

using namespace Rcpp;

PlEngine* engine = NULL ;

// [[Rcpp::export]]
int rolog_exit()
{
  if(engine)
    delete(engine) ;
  engine = NULL ;
  return 0 ;
} // rolog_exit

char* resfile[3] ;

// [[Rcpp::export]]
int rolog_init(String home, String prc)
{
  if(!engine)
  {
    resfile[0] = strdup((char*) (home.get_cstring())) ;
    resfile[1] = strdup((char*) "-x") ;
    resfile[2] = strdup((char*) (prc.get_cstring())) ;
    engine = new PlEngine(3, resfile) ;
  }
  // atexit(rolog_exit) ;
  return 0 ;
} // rolog_init
