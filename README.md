## *stable_vector\<T\>*

*(current status - very experimental and incomplete)*

A vector-like structure that guarantees that no elements
will be reallocated, and thus guarantee stable iterators
and, in fact, allow storing types that can neither be
copied nor moved.

`stable_vector<T>` does this by allocating blocks of
memory of increasing size as the vector grows.
