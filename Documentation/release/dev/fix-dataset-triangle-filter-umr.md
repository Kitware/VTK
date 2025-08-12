# Fix UMR in vtkDataSetTriangleFilter

For a vtkUnstructuredGrid input consisting of only tetrahedrons,
vtkDataSetTriangleFilter could give non-deterministic results due
to an uninitialized memory read (UMR).
