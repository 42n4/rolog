rcpp_hello_world <- function() 
{
  .Call('rolog_rcpp_hello_world', PACKAGE = 'rolog')
}

rolog_init <- function(home=system.file("swipl/lib/swipl-7.5.10", package="rolog"), prc=file.path(home, "boot64.prc"))
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

rolog_call <- function(prolog_call)
{
  .Call('rolog_rolog_call', prolog_call, PACKAGE = 'rolog')
}

rolog_query_LL <- function(predicate, arguments) 
{
  .Call('rolog_rolog_query_LL', enc2native(predicate), enc2native(arguments), PACKAGE='rolog')
}

rolog_query_comp <- function(predicate, arguments) 
{
  .Call('rolog_rolog_query_comp', enc2native(predicate), enc2native(arguments), PACKAGE='rolog')
}

rolog_query_string <- function(predicate, arguments) 
{
  .Call('rolog_rolog_query_string', predicate, arguments, PACKAGE = 'rolog')
}
