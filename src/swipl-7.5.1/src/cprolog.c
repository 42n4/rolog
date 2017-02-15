#include "SWI-Prolog.h"

int main(int argc, char** argv)
{
  char* resfile[3] ;
  resfile[0] = argv[0] ;
  resfile[1] = "-x" ;
  resfile[2] = "swipl.prc" ;

  if(!PL_initialise(3, resfile))
    PL_halt(1) ;

  for(;;)
  { 
    int status = PL_toplevel() ? 0 : 1 ;
    PL_halt(status) ;
  }

  return 0 ;
}


