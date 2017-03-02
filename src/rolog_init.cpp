#include <Rcpp.h>
#include "SWI-Prolog.h"
#include "SWI-cpp.h"

using namespace Rcpp;

PlEngine* rolog_engine = NULL ;

// [[Rcpp::export]]
void rolog_exit()
{
  if(rolog_engine)
    delete(rolog_engine) ;
  rolog_engine = NULL ;
} // rolog_exit

// [[Rcpp::export]]
int rolog_init(String home, String prc)
{
  if(!rolog_engine)
  {
    char* resfile[3] ;
    resfile[0] = (char*) (home.get_cstring()) ;
    resfile[1] = (char*) "-x" ;
    resfile[2] = (char*) (prc.get_cstring()) ;
    rolog_engine = new PlEngine(3, resfile) ;
  }
  
  char predicate[512] ;
  sprintf(predicate, "asserta(file_search_path(library, \"%s/library\"))", home.get_cstring()) ;
  PlCall(predicate) ;
  
  // atexit(rolog_exit) ;
  return 0 ;
} // rolog_init
