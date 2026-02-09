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
  strings, making case labels clear. (See Examples section below.)

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

It has only one configuration option which may be set by
consuming projects: `token_NAMESPACE`, which will change
the namespace containing the library's classes from
`token` to another valid identifier.

To build and test, simply run
```sh
cmake -G Ninja /path/to/token/source
ninja install
ctest
```

## Examples

### Creating and inspecting tokens

This example (see [basic.cxx](examples/basic.cxx))  will print the
string hash of the first command-line argument you pass to the program.
If none is passed, the "invalid" hash (i.e., the hash of an empty
string) is printed.  This example also shows how you can use
the `data()` and `getId()` methods on a token to retrieve either
a string with the matching token ID (if such a string exists) or
the token ID itself, respectively.

```cpp
  // Tokenize the first argument passed on the command line:
  token::Token userData(argc > 1 ? argv[1] : "");
  std::string input = userData.data();
  std::cout
    << '"' << input << '"' << " = 0x"
    << std::hex << userData.getId() << " = "
    << std::dec << userData.getId() << "\n"

    << "If userData was created at run-time:\n"
    << "  it should have data:" << (userData.hasData() ? "T" : "F") << " and,\n"
    << "  if non-empty, be valid:" << (userData.valid() ? "T" : "F") << ".\n";
```

Expected output for two runs:
```text
% ./bin/basic
Hash 2166136261 is missing from manager. Returning empty string.
"" = 0x811c9dc5 = 2166136261
If userData was created at run-time:
  it should have data:F and,
  if non-empty, be valid:F.
% ./bin/basic foo
"foo" = 0xa9f37ed7 = 2851307223
If userData was created at run-time:
  it should have data:T and,
  if non-empty, be valid:T.
```

### Comparing and storing tokens

Now let's consider how tokens are compared, which is especially
relevant to storing them in containers. They are not equivalent
to containers of strings.

If you use a sorted container (i.e., one that uses the less-than
operator for a comparator), then the underlying strings are
compared. This can be very inefficient, but allows you to create
a human-presentable list of tokens.

On the other hand, if you use an unordered container (i.e., one that
uses a hash function for a comparator), then the token IDs are
compared. This is very efficient (a single integer comparison vs.
an arbitrary-length string comparison) but does not make for good
presentation to humans.

The full code for this example is in [containers.cxx](examples/containers.cxx).

```cpp
  using namespace token;
  using namespace token::literals;

  // Using the less-than comparator causes underlying strings
  // (if they exist) to be sorted. All tokens with no underlying
  // string will evaluate as equivalent to an invalid token and
  // thus have a single container entry among them.
  std::set<Token> sorted{
    // These strings are turned into tokens at run-time:
    "foo", "bar", "baz",
    // These strings are turned into tokens at compile-time:
    "foo"_token, "xyzzy"_token
  };
  print_set(sorted, "sorted (and smushed)");

  // Using the hashing comparator causes token identifiers
  // to be indexed. Tokens with no underlying string will
  // have a separate entry, but it is not safe to assume their
  // underlying strings are unique (because some may be empty):
  std::unordered_set<Token> hashed{
    // These strings are turned into tokens at run-time:
    "foo", "bar", "baz",
    // These strings are turned into tokens at compile-time:
    "foo"_token, "xyzzy"_token, "same"_token, ""_token
  };
  print_set(hashed, "hashed (and maybe blank)");

  // Now let's insert strings into the token manager
  // (by hashing the same strings as above at run-time)
  // and re-print the contents of the set:
  Token dummy;
  dummy = "xyzzy";
  dummy = "same";
  print_set(hashed, "hashed (and no longer blank)");
```

Expected output of running this example:
```text
% ./bin/containers
Hash 628441653 is missing from manager. Returning empty string.
sorted (and smushed)
  0x25754235 ()
  0x76b77d1a (bar)
  0x6eb77082 (baz)
  0xa9f37ed7 (foo)
hashed (and maybe blank)
  0xcd0c4a3b ()
  0x25754235 ()
  0x811c9dc5 ()
  0x6eb77082 (baz)
  0x76b77d1a (bar)
  0xa9f37ed7 (foo)
hashed (and no longer blank)
  0xcd0c4a3b (same)
  0x25754235 (xyzzy)
  0x811c9dc5 ()
  0x6eb77082 (baz)
  0x76b77d1a (bar)
  0xa9f37ed7 (foo)
```

### Using tokens in switch statements

You can use token hashes as integer cases inside
`switch` statements to make your code more legible.

```cpp
  using namespace token::literals;
  // Tokenize the first argument passed on the command line:
  token::Token userData(argc > 1 ? argv[1] : "example");

  // Compare userData to some "expected" values:
  std::string message;
  switch (userData.getId())
  {
  case "foo"_hash: message = "bar is next"; break;
  case "bar"_hash: message = "baz is next"; break;
  case "baz"_hash: message = "foo is next"; break;
  default:
    message = "Unexpected option " + userData.data() + ".";
    break;
  }
  std::cout << message << "\n";
```

The full code for this example is in [switch-case.cxx](examples/switch-case.cxx).
Expected output of several runs:
```text
% ./bin/switch-case foo
bar is next
% ./bin/switch-case bar
baz is next
% ./bin/switch-case baz
foo is next
% ./bin/switch-case xyzzy
Unexpected option xyzzy.
```
