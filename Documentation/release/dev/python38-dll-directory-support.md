# Python 3.8 `os.add_dll_directory` support

VTK now has two new CMake options for deployments that can help to handle
Python 3.8's DLL loading policy changes.

  * `VTK_UNIFIED_INSTALL_TREE`: This option can be set to indicate that VTK
    will share an install tree with its dependencies. This allows VTK to avoid
    doing extra work that doesn't need to be done in such a situation. This is
    ignored if `VTK_RELOCATABLE_INSTALL` is given (there's no difference there
    as VTK assumes that how VTK is used in such a case is handled by other
    means).
  * `VTK_DLL_PATHS`: A list of paths to give to Python to use when loading DLL
    libraries.

Previously, VTK had added support for `vtkpython` to handle
`os.add_dll_directory`. VTK now handles it properly when `import vtkmodules` is
done from an external Python interpreter.
