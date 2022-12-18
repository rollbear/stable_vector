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

The data is structured as follows.

```
                               14[ ]
                               13[ ]
                               12[ ]
                               11[ ] <- v[11]
                          6[ ] 10[ ]
                          5[ ]  9[ ]
                  2[ ]    4[ ]  8[ ]
           0[ ]   1[ ]    3[ ]  7[ ]
             ^      ^      ^      ^
             |      |      |      |
vector -> [  |  ][  |  ][  |  ][  |  ]
             0      1      2      3
```

This allows for relatively fast indexing and iteration,
especially if the structure grows big. It also allows for
efficient growing in monotonic PMR allocators (not
implemented yet).

The lower vector will need to be reallocated as the number
of elements grows, but it will be increasingly rare, since
the storage space column will double in size for each
element that the vector grows.
