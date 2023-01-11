## Compile-time type names

VTK now provides a way to obtain type names at compile
time in the `Common/Core/vtkTypeName.h` header:

```c++
#include "vtkTypeName.h"

// ...
std::string typeName = vtk::TypeName<vtkImageData>();
std::cout << typeName << std::endl;

```

On most platforms, the example above will print `vtkImageData`.
This function uses the `abi::__cxa_demangle()` function provided
by many C++ compilers in `cxxabi.h`.
If that function is not provided, a mangled name will be returned
(which is the result of `typeid(ObjectType).name()`).

While `vtkObject::GetClassName()` is an alternative,
(1) it is a virtual method which requires an instance of the class to exist and
(2) it is unavailable for types which do not inherit `vtkObject`.
