## Improved Polyhedron Validation

New enumerations have been added to vtkCellStatus so that the cell
validator can properly report problematic behavior previously undetected:

+ Vertices considered coincident. This may occur either because the point
  tolerance in the validator was too large or because distinct IDs have the
  same coordinates.
  An automatic per-polyhedron tolerance is now provided as an option to the validator.
  It will choose a tolerance that is a fraction of the shortest non-zero
  edge length to prevent unwanted coincident vertices.
+ Inverted polyhedron faces are no longer reported as non-convex.
+ Faces which are collapsed to an edge or point are now detected.
+ Edges which are collapsed to a point are now detected.
