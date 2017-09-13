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
rolog_query(predicate, call)
}
\arguments{
  \item{predicate}{The predicate of the query.}
  \item{call}{Prolog compound.}
}
\examples{
\dontrun{
s = rolog_query("eval", 2 * sqr(2 + 3))
}
}
