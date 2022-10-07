## `vtkModuleWrapPython` header installation

When Python modules are built statically, there is a header that is generated
to initialize the builtin module table. This header had not been installed
previously. The `vtk_module_wrap_python(HEADERS_DESTINATION)` argument may now
be used to place these headers into the install tree.
