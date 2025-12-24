## vtkRedistributeDataSetFilter : fix missing cells in ASSIGN_TO_ONE_REGION mode

Concave cells that lie on the boundary of multiple partitions could be deleted in this mode.
Previously this use case was supported but has been removed during a refactor of this
class between vtk 9.2.0 and 9.3.0. This fix reintroduce this case.
