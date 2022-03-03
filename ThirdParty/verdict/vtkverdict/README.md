# Verdict
## Compute quality functions of 2 and 3-dimensional regions.

## Changelog

What's new for 1.4:
- Add inradius metrics for several element types
- Bug fixes

What's new for 1.3:
- Thread safety for all functions and simplified API
- Some existing metrics implemented for pyramid and wedge elements
- New inradius and timestep metrics
- New implementation for some higher order elements

## Building Verdict

To build with CMake

1. Set up a build directory
2. Change to this directory
3. Type `ccmake /path/to/verdict` but replace
   `/path/to/verdict` with the path to the directory
   containing this read-me file.
4. Fill the required fields and press the 'c' key
   NB: This process is iterative;
   you may need to change values and reconfigure before continuing.
5. When the 'g' option becomes available, press the 'g' key to generate
   the Makefile and exit CMake.
6. Build with `make`
7. Install with `make install`

## Using Verdict

If you do not already have verdict installed on your system,
see below for instructions to compile it.

Once you have verdict installed, you may use it in your
own project by adding

```cmake
  find_package(Verdict)
```

to your `CMakeLists.txt` file.

Now you can link your executable to the verdict library.
Assume you have an executable named `example`;
verdict uses CMake's _namespace_ feature, so you should
link to the verdict library like so:

```cmake
  target_link_libraries(example
    PUBLIC
      Verdict::verdict
  )
```

Note that executables do not need to specify `PUBLIC`, `PRIVATE`,
or `INTERFACE` linkage, but libraries must so that transitive
dependencies can be resolved by cmake.

`verdict.h` is the only header file you need to include.
