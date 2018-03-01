"""This is the vtk module."""
import sys

if sys.version_info < (3,5):
    # imp is deprecated in 3.4
    import imp, importlib

    # import vtkmodules package.
    vtkmodules_m = importlib.import_module('vtkmodules')

    # import vtkmodules.all
    all_m = importlib.import_module('vtkmodules.all')

    # create a clone of the `vtkmodules.all` module.
    vtk_m = imp.new_module(__name__)
    for key in dir(all_m):
        if not hasattr(vtk_m, key):
            setattr(vtk_m, key, getattr(all_m, key))

    # make the clone of `vtkmodules.all` act as a package at the same location
    # as vtkmodules. This ensures that importing modules from within the vtkmodules package
    # continues to work.
    vtk_m.__path__ = vtkmodules_m.__path__
    # replace old `vtk` module with this new package.
    sys.modules[__name__] = vtk_m

else:
    import importlib.util
    # import vtkmodules.all
    all_spec = importlib.util.find_spec('vtkmodules.all')
    all_m = importlib.util.module_from_spec(all_spec)
    all_spec.loader.exec_module(all_m)

    # import vtkmodules
    vtkmodules_spec = importlib.util.find_spec('vtkmodules')
    vtkmodules_m = importlib.util.module_from_spec(vtkmodules_spec)
    vtkmodules_spec.loader.exec_module(vtkmodules_m)

    # make vtkmodules.all act as the vtkmodules package to support importing
    # other modules from vtkmodules package via `vtk`.
    all_m.__path__ = vtkmodules_m.__path__

    del all_spec, vtkmodules_spec

    # replace old `vtk` module with the `all` package.
    sys.modules[__name__] = all_m
