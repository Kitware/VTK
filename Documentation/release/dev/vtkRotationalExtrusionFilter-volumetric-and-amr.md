# `vtkRotationalExtrusionFilter` supports volumetric and AMR inputs

`vtkRotationalExtrusionFilter` now supports volumetric rotational extrusion in
addition to its existing surface extrusion capabilities. The filter incorporates
the functionality of `vtkVolumeOfRevolutionFilter`, providing a unified
interface for surface and volumetric rotational extrusion.

The filter also supports AMR datasets by converting the AMR input to an
unstructured grid for volumetric extrusion.

New options include `RotationAxis`, `AxisPosition`, and `Volumetric`, allowing
the rotation axis to be configured and the desired extrusion mode to be
selected.

`vtkVolumeOfRevolutionFilter` remains available and is not deprecated by this
change.

# Non-retrocompatible changes

In order to provide this support, the inherited algorithm of the class
was changed from vtkPolyDataAlgorithm to vtkDataObjectAlgorithm, a more
general algorithm type to support vtkUnstructuredGrid outputs. This
is technically non-retrocompatible.
