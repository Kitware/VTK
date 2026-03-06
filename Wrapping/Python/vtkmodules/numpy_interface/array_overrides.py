"""Register numpy-compatible overrides for VTK array types.

Importing this module registers VTKAOSArray and VTKSOAArray mixins as
overrides for all vtkAOSDataArrayTemplate and vtkSOADataArrayTemplate
instantiations.  This is triggered automatically when vtkCommonCore is
imported (via MODULE_MAPPER in vtkmodules/__init__.py).
"""
from contextlib import suppress

with suppress(ImportError):
    from vtkmodules.numpy_interface import vtk_aos_array as _aos  # noqa: F401
    from vtkmodules.numpy_interface import vtk_soa_array as _soa  # noqa: F401
    from vtkmodules.numpy_interface import vtk_constant_array as _const  # noqa: F401
    from vtkmodules.numpy_interface import vtk_affine_array as _affine  # noqa: F401
    from vtkmodules.numpy_interface import vtk_strided_array as _stride  # noqa: F401
    from vtkmodules.numpy_interface import vtk_indexed_array as _idx  # noqa: F401
    from vtkmodules.numpy_interface import vtk_composite_array as _comp  # noqa: F401
    from vtkmodules.numpy_interface import vtk_structured_point_array as _sp  # noqa: F401
    # Fallback for implicit arrays and any other vtkDataArray subclass
    # without a dedicated override.  Must be imported AFTER AOS/SOA so
    # that their more-specific overrides are registered first.
    from vtkmodules.numpy_interface import vtk_implicit_array as _implicit  # noqa: F401
