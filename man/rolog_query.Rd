\name{rolog_query}
\alias{rolog_query}
\docType{package}
\title{
Invoke a Prolog query
}
\description{
Invoke a Prolog query and return a character vector with all solutions.
}
\usage{
rolog_query(predicate, arguments)	
}
\arguments{
  \item{predicate}{The predicate of the query.}
  \item{arguments}{Currently, only fully instantiated arguments are supported.}
}
\examples{
\dontrun{
s = rolog_query("eval", "2 * sqr(2 + 3)")
}
}
