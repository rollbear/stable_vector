## *stable_vector\<T\>*

[![CI Build Status](https://github.com/rollbear/stable_vector/actions/workflows/ci.yml/badge.svg)](https://github.com/rollbear/stable_vector/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/rollbear/stable_vector/branch/main/graph/badge.svg?token=cS97dA6jRV)](https://codecov.io/gh/rollbear/stable_vector)

*(current status - very experimental and incomplete)*

A vector-like structure that guarantees that no elements
will be reallocated, and thus guarantee stable iterators
and, in fact, allow storing types that can neither be
copied nor moved.

`stable_vector<T>` does this by allocating blocks of
memory of increasing size as the vector grows.
