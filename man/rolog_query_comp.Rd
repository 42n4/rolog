\name{rolog_query_comp}
\alias{rolog_query_comp}
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
  \item{arguments}{Prolog compound.}
}
\examples{
\dontrun{
s = rolog_query_comp("eval", "2 * sqr(2 + 3)")
}
}
