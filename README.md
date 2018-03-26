# BE-Tree

## Questions
* For strings, should we use the q-gram thing mentionned in the paper, or make the splitting strategy different (ie. if start by. there might never be an bucket of a single value doe, unless we configure like "split max for 2 letters, etc)? Could also use all the expressions to make a hash table and split this way, since we would never match on anything beside some strings if we always do exact string matches + one extra value for anything else for non-equality.