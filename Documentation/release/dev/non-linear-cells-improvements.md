## vtkNonLinearCell: Add Drawing, Add Edge Queries, and Implement Topology/Inflation API

The code of all non-linear cells has been cleaned up, and the following improvements have been made:

1. All non-linear cells now have a drawing showing the numbering of their points.
2. Several non-linear cells (`vtkQuadraticEdge`, `vtkQuadraticTriangle`, `vtkQuadraticQuad`,
   `vtkBiQuadraticQuad`, `vtkBiQuadraticTriangle`, `vtkCubicLine`) now have edge definitions.
3. `vtkQuadraticEdge` now has a `Derivatives` method implemented.
4. `vtkNonLinearCell3D` now provides the full topology query API similar to `vtkCell3D`:
   `GetFacePoints`, `GetEdgePoints`, `GetEdgeToAdjacentFaces`, `GetFaceToAdjacentFaces`,
   `GetPointToIncidentEdges`, `GetPointToIncidentFaces`, and `GetPointToOneRingPoints`.
   These are implemented for all 3D non-linear cells:
   `vtkQuadraticTetra`, `vtkQuadraticHexahedron`, `vtkQuadraticWedge`, `vtkQuadraticPyramid`,
   `vtkQuadraticLinearWedge`, `vtkTriQuadraticHexahedron`, `vtkTriQuadraticPyramid`,
   `vtkBiQuadraticQuadraticWedge`, and `vtkBiQuadraticQuadraticHexahedron`.
5. `vtkPolygon` now has a `ComputeArea` static method that uses Newell's method to compute
   the area and area-weighted normal of an arbitrary polygon in 3D.
6. `vtkNonLinearCell3D` now implements `IsInsideOut()` and `Inflate(double dist)`:
   - `IsInsideOut()` evaluates the sign of the Jacobian determinant at the parametric center.
   - `Inflate()` displaces each control point outward along the world-space normal direction
     derived from its incident parametric-space face normals via Nanson's formula. Singular
     Jacobians (e.g. the quadratic-pyramid apex) are detected and those points are left in place.
