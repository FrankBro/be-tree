# BE-Tree

## To clarify
* In the flights, I see a few expressions which compare strings that are always integers, can we convert them? Example: pc
* In the flights, sometimes string comparisons could be an enum, can we convert them? Example: countries/regions, position

## Questions
* For strings, should we use the q-gram thing mentionned, or make the splitting strategy different (ie. if start by. there might never be an bucket of a single value doe, unless we configure like "split max for 2 letters, etc)