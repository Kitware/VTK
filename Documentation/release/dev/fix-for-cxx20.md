## Fixes for C++20

Several fixes to allow VTK to build with gcc11 -std=c++20

* C++20 does not like (enum * float) operations, forced to int*float.
* Enforce char[256] function param.
* Fixup lambdas that capture 'this' ('this' is no longer captured implicitly with =).

And a bonus bugfix: Don't call GetClassName() on an null pointer.
