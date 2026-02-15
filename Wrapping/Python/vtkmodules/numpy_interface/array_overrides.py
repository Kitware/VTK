"""Register numpy-compatible overrides for VTK array types.

Importing this module registers VTKAOSArray, VTKSOAArray,
VTKConstantArray, and VTKAffineArray mixins as overrides for all
vtkAOSDataArrayTemplate, vtkSOADataArrayTemplate, vtkConstantArray,
and vtkAffineArray instantiations.  This is triggered automatically
when vtkCommonCore is imported (via MODULE_MAPPER in vtkmodules/__init__.py).
"""
from contextlib import suppress

with suppress(ImportError):
    from vtkmodules.numpy_interface import vtk_aos_array as _aos  # noqa: F401
    from vtkmodules.numpy_interface import vtk_soa_array as _soa  # noqa: F401
    from vtkmodules.numpy_interface import vtk_constant_array as _const  # noqa: F401
    from vtkmodules.numpy_interface import vtk_affine_array as _affine  # noqa: F401
