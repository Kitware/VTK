# Token: Utilities for string tokenization

String tokenization is the process of assigning a unique
integer token to an input string.
This is useful in several situations

+ **String comparison**. If performed repeatedly, comparison costs up to O(n)
  each call (where n is the minimum length of any pair of input strings).
  Token comparison, if performed repeatedly, is O(1).
+ **String storage**. If the same string is stored in many places (say with
  arrays of strings being processed), it can consume significant memory.
  Tokens are fixed-size references to strings, limiting the memory overhead.
+ **String interning**. Libraries used in security-significant settings can
  refuse to provide a token for an "unsafe" string (or instead provide a
  token for properly-escaped counterpart). By forcing the API to refer to
  tokens rather than strings, some safety is provided. (In this situation,
  you must be careful to deal with collisions and with how strings are
  inserted into storage for retrieval from a token.)
+ **Enumerations**. Traditional C++ enumerations are fixed at compile time.
  This can be problematic in libraries that wish to provide extensible
  enumerations. Tokens can be used in place of enumerants in many cases
  while allowing the set of interned strings to grow. One particular use
  case of interest is `switch(…) { … }` statements; from C++14 onwards, it
  is possible for `case` statements to refer to compile-time hashes of
  strings, making case labels clear.

## Tokenization

The `token` library provides:

+ A `token::Token` class that can be constructed from either a string
  or an integer hash. Hashes may be computed at compile time, though
  + compile-time hashes cannot be checked for collisions and
  + compile-time hashes do not have their corresponding strings interned.
+ A `token::Manager` class that holds interned strings.
  An instance of the manager is owned as a class-static member of
  `token::Token` so that all interned strings are collected by one
  object.
+ String-literal operators for hashing strings
  + The `""_token` operator produces a `token::Token` instance from a string.
  + The `""_hash` operator produces a `token::Hash` integer from a string.

Tokenization (computing a hash of the string) is performed with the
[FNV-1a algorithm](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV-1a_hash).

A simple utility program, named `tokenize`, is provided with the
library. Given one or more strings on the command line, it computes
the integer hash of each and reports them. The hash number is
identical on all platforms
```sh
% ./bin/tokenize "" a b c ab bc ac abc
0x811c9dc5 = ""
0xe40c292c = "a"
0xe70c2de5 = "b"
0xe60c2c52 = "c"
0x4d2505ca = "ab"
0x3e2ba9f2 = "bc"
0x4e25075d = "ac"
0x1a47e90b = "abc"
```

## Additional features

Besides string tokenization, the `token` library provides

+ Templated `typeName<T>()` and `typeToken<T>()` functions that
  return a string (respectively `Token`) holding the type-name
  of the template parameter `T`.
  In the future, these functions will be `constexpr`, but are not
  for now because older compilers do not allow `constexpr` functions
  with temporary variables.
+ A TypeContainer class for registering and fetching singleton
  objects given their type as a template parameter.
  (A string token of each object's type-name is used as the
  key into an unordered map of object-wrappers.)
+ A singleton API that provides a global instance of a
  TypeContainer for applications store/retrieve singletons
  of any type.
  This functionality is provided since the `token` library
  must be dynamic (it has a global variable holding a
  `token::Manager` instance), so it may as well provide this
  service to others.
+ Serialization/deserialization of tokens and their interned
  strings to/from JSON.

## Building

The `token` library depends on the
[`nlohmann_json`](https://github.com/nlohmann/json) library.

It has only configuration option which may be set by
consuming projects: `token_NAMESPACE`, which will change
the namespace containing the library's classes from
`token` to another valid identifier.

To build and test, simply run
```sh
cmake -G Ninja /path/to/token/source
ninja install
ctest
```
