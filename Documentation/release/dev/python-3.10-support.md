## Python 3.10 support

VTK now supports changes made in Python 3.10 related to interpreter
initialization. Before this release, `vtkpython` could not load `vtkmodules` by
default due to these behavior changes.
