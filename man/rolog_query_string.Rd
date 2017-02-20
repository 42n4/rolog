\name{rolog_query_string}
\alias{rolog_query_string}
\docType{package}
\title{
Invoke a Prolog query
}
\description{
Invoke a Prolog query and return a character vector with all solutions.
}
\usage{
rolog_query_string(predicate, arguments)
}
\arguments{
  \item{predicate}{The predicate of the query.}
  \item{arguments}{Prolog string.}
}
\examples{
\dontrun{
s = rolog_query_string("eval", "2 * sqr(2 + 3)")
}
}
