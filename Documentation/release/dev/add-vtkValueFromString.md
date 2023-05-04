## Add vtkValueFromString

`vtkValueFromString` is a new low-level function that converts a string to an integer, a floating-point value or a boolean.

`vtkValueFromString` is faster than standard library functions such as `std::strto*` functions family:
- Parsing of floating-point values is about 4 times faster than `std::strto[d/f]` thanks to [fast_float](https://github.com/fastfloat/fast_float).
- Parsing of integer values is about 20% faster than `std::strtol`, this algorithm is based on [scnlib](https://github.com/eliaskosunen/scnlib) algorithm.
- Parsing of boolean is not comparable to any standard function.
