"""This is the vtk module."""
import sys, importlib

vtkmodules = importlib.import_module("vtkmodules")
vtkmodules_all = importlib.import_module("vtkmodules.all")
# merge components from `all` to `vtkmodules` package.
for key in dir(vtkmodules_all):
    if not hasattr(vtkmodules, key):
        setattr(vtkmodules, key, getattr(vtkmodules_all, key))

# replace ourselves with the `vtkmodules` package. This is essential
# to ensure the importing of other modules/packages from `vtkmodules`
# works seamlessly when accessed via `vtk` pseudo-package.
sys.modules[__name__] = vtkmodules
