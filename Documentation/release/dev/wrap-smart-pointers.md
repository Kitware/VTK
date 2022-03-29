# Wrap methods with smart pointers in Python

Methods that contain `vtkSmartPointer<T>` as a parameter or a return value
will be wrapped for Python, and the smart pointers will be automatically
converted to (or from) Python-wrapped VTK objects.  The wrappers will also
allow `std::vector<vtkSmartPointer<T>>`, which will be converted to a tuple
of VTK objects when returned from C++ to Python, and will be converted from
any sequence type (tuple, list, etc) when passed from Python to C++.
