## Addition of `vtkGetEnumMacro/vtkSetEnumMacro` for `enum class`

Since C++11 `enum class` does not implicitly convert to integers, the existing `vtkGetMacro/vtkSetMacro` can't work with them.

Added two new macros `vtkGetEnumMacro/vtkSetEnumMacro` that do work.
