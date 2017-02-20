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

// [[Rcpp::export]]
int rolog_init(String home, String prc)
{
  if(!engine)
  {
    char* resfile[3] ;
    resfile[0] = (char*) (home.get_cstring()) ;
    resfile[1] = (char*) "-x" ;
    resfile[2] = (char*) (prc.get_cstring()) ;
    engine = new PlEngine(3, resfile) ;
  }
  // atexit(rolog_exit) ;
  return 0 ;
} // rolog_init
