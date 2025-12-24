## Refactor vtkHyperTree class to simplify it

Address https://gitlab.kitware.com/vtk/vtk/-/issues/19724

vtkHyperTree is now an instantiable class.

It was previously an abstract class, for which the concrete implementation was hidden in vtkCompactHyperTree.
However, in a decade of existence, there has not been another internal HyperTree implementation.
Therefore, all the implementations of vtkCompactHyperTree have been moved to vtkHyperTree.

A few "private" methods were removed:
- InitializePrivate
- PrintSelfPrivate
- CopyStructurePrivate

"Scales" member was moved to private.
