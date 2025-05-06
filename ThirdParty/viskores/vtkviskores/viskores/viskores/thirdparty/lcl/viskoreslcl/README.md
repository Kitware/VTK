# Lightweight Cell Library #

Lightweight Cell Library is a collection of cell types and cell functionality
that is designed be used scientific visualization libraries of
Viskores, and VTK.

You can find out more about the design of Lightweight Cell Library in [DESIGN.md].

## Contributing ##

There are many ways to contribute to [LCL]:

  + Submit new or add to discussions of a feature requests or bugs on the
    [LCL Issue Tracker].

  + Submit a Pull Request
      + See [CONTRIBUTING.md] for detailed instructions on how to create a
        Pull Request.
      + See the [LCL Coding Conventions] that must be followed for
        contributed code.

## Compiler Requirements ##

  + C++11 Compiler. Lightweight Cell Library has been confirmed to work with
    the following
      + GCC 5.4+
      + Clang 3.3+
      + XCode 5.0+
      + MSVC 2015+

## Example ##

Below is a simple example of using Lightweight Cell Library to get derivatives
and parametric coordinates for different cell types:

```cpp
#incude <lcl/lcl.h>

std::array<float, 3> pcoords;
auto status = lcl::parametricCenter(lcl::Hexahedron{}, pcoords);

std::vector<std::array<float, 3>> points = { ... };
std::vector<std::array<double, 4>> field = { ... };
std::array<double, 4> derivs[3];
status = lcl::derivative(lcl::Hexahedron{},
                          lcl::makeFieldAccessorNestedSOAConst(points, 3),
                          lcl::makeFieldAccessorNestedSOAConst(field, 4),
                          pcoords,
                          derivs[0],
                          derivs[1],
                          derivs[2]);
```

## License ##

Lightweight Cell Library is distributed under the OSI-approved BSD 3-clause
License. See [LICENSE.md] for details.


[LCL]:                                       https://gitlab.kitware.com/vtk/lcl/
[LCL Issue Tracker]:                   https://gitlab.kitware.com/vtk/lcl/issues

[CONTRIBUTING.md]:          CONTRIBUTING.md
[DESIGN.md]:                docs/Design.md
[LICENSE.md]:               LICENSE.md
[LCL Coding Conventions]: docs/CodingConventions.md
