## ABI Mangling extention for C/C++ ABI in VTK

Using the C++ feature `inline namespace` VTK is able to apply ABI mangling
to VTK without affecting the API interface in code. This feature allows for
separately compiled VTK libaries isolated in their own translation units to
be linked in the same application without symbol conflicts at runtime.

This feature was "experimental" due to lack of  CI testing and
missing manglings for C ABIs in VTK. Both of these missing components are
now added.

The C ABIs previously skipped my the ABI mangling feature are now included.
Most of the C ABIs have added macros for compatibility in VTK module code.
While the macros allow for use of the unmangled name in code, they do not
allow for loading functions via the DLL interface by the unmangled names.

One exception to the compatibility macros is the GetVTKVersion ABI which
conflicts with the `vtkVersion::GetVTKVersion` name.

Notes:
* This change does not affect VTK builds that are not using the `VTK_ABI_NAMESPACE` feature.
* Thirdparty libraries still do not support ABI mangling.
* VTK Python does not support ABI mangling.
