rcpp_hello_world <- function() 
{
  .Call('rolog_rcpp_hello_world', PACKAGE = 'rolog')
}

rolog_init <- function(home=system.file("swipl/lib/swipl-7.5.1/boot64.prc", package="rolog"), prc=home)
{
  .Call('rolog_rolog_init', home, prc, PACKAGE = 'rolog')
}

rolog_exit <- function() 
{
  .Call('rolog_rolog_exit', PACKAGE = 'rolog')
}

rolog_consult <- function(prolog_files)
{
  .Call('rolog_rolog_consult', prolog_files, PACKAGE = 'rolog')
}

rolog_query_comp <- function(predicate, arguments) 
{
  .Call('rolog_rolog_query_comp', predicate, arguments, PACKAGE = 'rolog')
}
