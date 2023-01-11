## Compile-time string hashing

The `vtkStringToken` class introduces a utility for hashing
strings at either compile or run-time and using the resulting
integers as tokens.

Besides the `vtkStringToken` class, there is a `vtkStringManager`
used to hold strings hashed at **runtime** (i.e., not at compile
time). This makes it possible for the string-token class to
return the original string to you in some cases. Because the
manager holds a map from string-hash to string, only a single
copy of the string is stored no matter how many copies of the
token exist.

Finally, there are two string-literal operators added to
a new `vtk::literals` namespace for creating hashes and
tokens at compile time:

+ `""_hash` – returns a 32-bit integer hash of the given string.
+ `""_token` – returns a `vtkStringToken` instance of the
  given string. Note that because the hash is computed during
  compilation, you may not call the token's `Data()` method
  to retrieve the string unless it is inserted at run time by
  some other code.

### Intended uses

There are some motivating use cases for string tokenization:

+ **Switch statements with "string" case-labels.**
  Because hashing can occur at build time, this is possible:
  ```cpp
  #include "vtkStringToken.h"
  using namespace vtk::literals;
  vtkStringToken t;
  switch (t.GetId())
  {
    case "foo"_hash: foo(); break;
    case "bar"_hash: bar(); break;
    default: vtkErrorMacro("Unknown token " << t.Data()); break;
  }
  ```

+ **Coloring by strings.**
  While VTK provides some facilities for coloring data by
  `vtkStringArray` values, converting the string array to
  integer values is much more efficient and amenable to
  transferring data for GPU rendering/processing.

+ **Extensible enumerations.**
  Some upcoming contributions to VTK to support datasets
  in a more extensible way will use string tokens to
  maintain legibility while avoiding arbitrary integer
  constants or inflexible enumerations.
