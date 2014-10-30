package require vtk
package require vtkinteraction

# Create 2D and 3D quadratic cells and extract their edges

# Quadratic triangle
vtkPoints triPoints
  triPoints SetNumberOfPoints 6
  triPoints InsertPoint 0 2.0 0.0 0.0
  triPoints InsertPoint 1 3.0 0.0 0.0
  triPoints InsertPoint 2 2.5 0.8 0.0
  triPoints InsertPoint 3 2.5 0.0 0.0
  triPoints InsertPoint 4 2.75 0.4 0.0
  triPoints InsertPoint 5 2.25 0.4 0.0
vtkFloatArray triScalars
  triScalars SetNumberOfTuples 6
  triScalars InsertValue 0 0.0
  triScalars InsertValue 1 0.0
  triScalars InsertValue 2 0.0
  triScalars InsertValue 3 1.0
  triScalars InsertValue 4 1.0
  triScalars InsertValue 5 0.0
vtkQuadraticTriangle aTri
  [aTri GetPointIds] SetId 0 0
  [aTri GetPointIds] SetId 1 1
  [aTri GetPointIds] SetId 2 2
  [aTri GetPointIds] SetId 3 3
  [aTri GetPointIds] SetId 4 4
  [aTri GetPointIds] SetId 5 5
vtkUnstructuredGrid aTriGrid
  aTriGrid Allocate 1 1
  aTriGrid InsertNextCell [aTri GetCellType] [aTri GetPointIds]
  aTriGrid SetPoints triPoints
  [aTriGrid GetPointData] SetScalars triScalars

# Quadratic quadrilateral
vtkPoints quadPoints
  quadPoints SetNumberOfPoints 8
  quadPoints InsertPoint 0 4.0 0.0 0.0
  quadPoints InsertPoint 1 5.0 0.0 0.0
  quadPoints InsertPoint 2 5.0 1.0 0.0
  quadPoints InsertPoint 3 4.0 1.0 0.0
  quadPoints InsertPoint 4 4.5 0.0 0.0
  quadPoints InsertPoint 5 5.0 0.5 0.0
  quadPoints InsertPoint 6 4.5 1.0 0.0
  quadPoints InsertPoint 7 4.0 0.5 0.0
vtkFloatArray quadScalars
  quadScalars SetNumberOfTuples 8
  quadScalars InsertValue 0 0.0
  quadScalars InsertValue 1 0.0
  quadScalars InsertValue 2 1.0
  quadScalars InsertValue 3 1.0
  quadScalars InsertValue 4 1.0
  quadScalars InsertValue 5 0.0
  quadScalars InsertValue 6 0.0
  quadScalars InsertValue 7 0.0
vtkQuadraticQuad aQuad
  [aQuad GetPointIds] SetId 0 0
  [aQuad GetPointIds] SetId 1 1
  [aQuad GetPointIds] SetId 2 2
  [aQuad GetPointIds] SetId 3 3
  [aQuad GetPointIds] SetId 4 4
  [aQuad GetPointIds] SetId 5 5
  [aQuad GetPointIds] SetId 6 6
  [aQuad GetPointIds] SetId 7 7
vtkUnstructuredGrid aQuadGrid
  aQuadGrid Allocate 1 1
  aQuadGrid InsertNextCell [aQuad GetCellType] [aQuad GetPointIds]
  aQuadGrid SetPoints quadPoints
  [aQuadGrid GetPointData] SetScalars quadScalars

# BiQuadratic quadrilateral
vtkPoints BquadPoints
  BquadPoints SetNumberOfPoints 9
  BquadPoints InsertPoint 0 4.0 2.0 0.0
  BquadPoints InsertPoint 1 5.0 2.0 0.0
  BquadPoints InsertPoint 2 5.0 3.0 0.0
  BquadPoints InsertPoint 3 4.0 3.0 0.0
  BquadPoints InsertPoint 4 4.5 2.0 0.0
  BquadPoints InsertPoint 5 5.0 2.5 0.0
  BquadPoints InsertPoint 6 4.5 3.0 0.0
  BquadPoints InsertPoint 7 4.0 2.5 0.0
  BquadPoints InsertPoint 8 4.5 2.5 0.0
vtkFloatArray BquadScalars
  BquadScalars SetNumberOfTuples 9
  BquadScalars InsertValue 0 1.0
  BquadScalars InsertValue 1 1.0
  BquadScalars InsertValue 2 1.0
  BquadScalars InsertValue 3 1.0
  BquadScalars InsertValue 4 0.0
  BquadScalars InsertValue 5 0.0
  BquadScalars InsertValue 6 0.0
  BquadScalars InsertValue 7 0.0
  BquadScalars InsertValue 8 0.0
vtkBiQuadraticQuad BQuad
  [BQuad GetPointIds] SetId 0 0
  [BQuad GetPointIds] SetId 1 1
  [BQuad GetPointIds] SetId 2 2
  [BQuad GetPointIds] SetId 3 3
  [BQuad GetPointIds] SetId 4 4
  [BQuad GetPointIds] SetId 5 5
  [BQuad GetPointIds] SetId 6 6
  [BQuad GetPointIds] SetId 7 7
  [BQuad GetPointIds] SetId 8 8
vtkUnstructuredGrid BQuadGrid
  BQuadGrid Allocate 1 1
  BQuadGrid InsertNextCell [BQuad GetCellType] [BQuad GetPointIds]
  BQuadGrid SetPoints BquadPoints
  [BQuadGrid GetPointData] SetScalars BquadScalars


# Quadratic linear quadrilateral
vtkPoints QLquadPoints
  QLquadPoints SetNumberOfPoints 6
  QLquadPoints InsertPoint 0 4.0 4.0 0.0
  QLquadPoints InsertPoint 1 5.0 4.0 0.0
  QLquadPoints InsertPoint 2 5.0 5.0 0.0
  QLquadPoints InsertPoint 3 4.0 5.0 0.0
  QLquadPoints InsertPoint 4 4.5 4.0 0.0
  QLquadPoints InsertPoint 5 4.5 5.0 0.0
vtkFloatArray QLquadScalars
  QLquadScalars SetNumberOfTuples 6
  QLquadScalars InsertValue 0 1.0
  QLquadScalars InsertValue 1 1.0
  QLquadScalars InsertValue 2 1.0
  QLquadScalars InsertValue 3 1.0
  QLquadScalars InsertValue 4 0.0
  QLquadScalars InsertValue 5 0.0
vtkQuadraticLinearQuad QLQuad
  [QLQuad GetPointIds] SetId 0 0
  [QLQuad GetPointIds] SetId 1 1
  [QLQuad GetPointIds] SetId 2 2
  [QLQuad GetPointIds] SetId 3 3
  [QLQuad GetPointIds] SetId 4 4
  [QLQuad GetPointIds] SetId 5 5
vtkUnstructuredGrid QLQuadGrid
  QLQuadGrid Allocate 1 1
  QLQuadGrid InsertNextCell [QLQuad GetCellType] [QLQuad GetPointIds]
  QLQuadGrid SetPoints QLquadPoints
  [QLQuadGrid GetPointData] SetScalars QLquadScalars


# Quadratic tetrahedron
vtkPoints tetPoints
  tetPoints SetNumberOfPoints 10
  tetPoints InsertPoint 0 6.0 0.0 0.0
  tetPoints InsertPoint 1 7.0 0.0 0.0
  tetPoints InsertPoint 2 6.5 0.8 0.0
  tetPoints InsertPoint 3 6.5 0.4 1.0
  tetPoints InsertPoint 4 6.5 0.0 0.0
  tetPoints InsertPoint 5 6.75 0.4 0.0
  tetPoints InsertPoint 6 6.25 0.4 0.0
  tetPoints InsertPoint 7 6.25 0.2 0.5
  tetPoints InsertPoint 8 6.75 0.2 0.5
  tetPoints InsertPoint 9 6.50 0.6 0.5
vtkFloatArray tetScalars
  tetScalars SetNumberOfTuples 10
  tetScalars InsertValue 0 1.0
  tetScalars InsertValue 1 1.0
  tetScalars InsertValue 2 1.0
  tetScalars InsertValue 3 1.0
  tetScalars InsertValue 4 0.0
  tetScalars InsertValue 5 0.0
  tetScalars InsertValue 6 0.0
  tetScalars InsertValue 7 0.0
  tetScalars InsertValue 8 0.0
  tetScalars InsertValue 9 0.0
vtkQuadraticTetra aTet
  [aTet GetPointIds] SetId 0 0
  [aTet GetPointIds] SetId 1 1
  [aTet GetPointIds] SetId 2 2
  [aTet GetPointIds] SetId 3 3
  [aTet GetPointIds] SetId 4 4
  [aTet GetPointIds] SetId 5 5
  [aTet GetPointIds] SetId 6 6
  [aTet GetPointIds] SetId 7 7
  [aTet GetPointIds] SetId 8 8
  [aTet GetPointIds] SetId 9 9
vtkUnstructuredGrid aTetGrid
  aTetGrid Allocate 1 1
  aTetGrid InsertNextCell [aTet GetCellType] [aTet GetPointIds]
  aTetGrid SetPoints tetPoints
  [aTetGrid GetPointData] SetScalars tetScalars

# Quadratic hexahedron
vtkPoints hexPoints
  hexPoints SetNumberOfPoints 20
  hexPoints InsertPoint 0 8 0 0
  hexPoints InsertPoint 1 9 0 0
  hexPoints InsertPoint 2 9 1 0
  hexPoints InsertPoint 3 8 1 0
  hexPoints InsertPoint 4 8 0 1
  hexPoints InsertPoint 5 9 0 1
  hexPoints InsertPoint 6 9 1 1
  hexPoints InsertPoint 7 8 1 1
  hexPoints InsertPoint 8 8.5 0 0
  hexPoints InsertPoint 9 9 0.5 0
  hexPoints InsertPoint 10 8.5 1 0
  hexPoints InsertPoint 11 8 0.5 0
  hexPoints InsertPoint 12 8.5 0 1
  hexPoints InsertPoint 13 9 0.5 1
  hexPoints InsertPoint 14 8.5 1 1
  hexPoints InsertPoint 15 8 0.5 1
  hexPoints InsertPoint 16 8 0 0.5
  hexPoints InsertPoint 17 9 0 0.5
  hexPoints InsertPoint 18 9 1 0.5
  hexPoints InsertPoint 19 8 1 0.5
vtkFloatArray hexScalars
  hexScalars SetNumberOfTuples 20
  hexScalars InsertValue 0 1.0
  hexScalars InsertValue 1 1.0
  hexScalars InsertValue 2 1.0
  hexScalars InsertValue 3 1.0
  hexScalars InsertValue 4 1.0
  hexScalars InsertValue 5 1.0
  hexScalars InsertValue 6 1.0
  hexScalars InsertValue 7 1.0
  hexScalars InsertValue 8 0.0
  hexScalars InsertValue 9 0.0
  hexScalars InsertValue 10 0.0
  hexScalars InsertValue 11 0.0
  hexScalars InsertValue 12 0.0
  hexScalars InsertValue 13 0.0
  hexScalars InsertValue 14 0.0
  hexScalars InsertValue 15 0.0
  hexScalars InsertValue 16 0.0
  hexScalars InsertValue 17 0.0
  hexScalars InsertValue 18 0.0
  hexScalars InsertValue 19 0.0
vtkQuadraticHexahedron aHex
  [aHex GetPointIds] SetId 0 0
  [aHex GetPointIds] SetId 1 1
  [aHex GetPointIds] SetId 2 2
  [aHex GetPointIds] SetId 3 3
  [aHex GetPointIds] SetId 4 4
  [aHex GetPointIds] SetId 5 5
  [aHex GetPointIds] SetId 6 6
  [aHex GetPointIds] SetId 7 7
  [aHex GetPointIds] SetId 8 8
  [aHex GetPointIds] SetId 9 9
  [aHex GetPointIds] SetId 10 10
  [aHex GetPointIds] SetId 11 11
  [aHex GetPointIds] SetId 12 12
  [aHex GetPointIds] SetId 13 13
  [aHex GetPointIds] SetId 14 14
  [aHex GetPointIds] SetId 15 15
  [aHex GetPointIds] SetId 16 16
  [aHex GetPointIds] SetId 17 17
  [aHex GetPointIds] SetId 18 18
  [aHex GetPointIds] SetId 19 19
vtkUnstructuredGrid aHexGrid
  aHexGrid Allocate 1 1
  aHexGrid InsertNextCell [aHex GetCellType] [aHex GetPointIds]
  aHexGrid SetPoints hexPoints
  [aHexGrid GetPointData] SetScalars hexScalars

# TriQuadratic hexahedron
vtkPoints TQhexPoints
  TQhexPoints SetNumberOfPoints 27
  TQhexPoints InsertPoint 0 8 2 0
  TQhexPoints InsertPoint 1 9 2 0
  TQhexPoints InsertPoint 2 9 3 0
  TQhexPoints InsertPoint 3 8 3 0
  TQhexPoints InsertPoint 4 8 2 1
  TQhexPoints InsertPoint 5 9 2 1
  TQhexPoints InsertPoint 6 9 3 1
  TQhexPoints InsertPoint 7 8 3 1
  TQhexPoints InsertPoint 8 8.5 2 0
  TQhexPoints InsertPoint 9 9 2.5 0
  TQhexPoints InsertPoint 10 8.5 3 0
  TQhexPoints InsertPoint 11 8 2.5 0
  TQhexPoints InsertPoint 12 8.5 2 1
  TQhexPoints InsertPoint 13 9 2.5 1
  TQhexPoints InsertPoint 14 8.5 3 1
  TQhexPoints InsertPoint 15 8 2.5 1
  TQhexPoints InsertPoint 16 8 2 0.5
  TQhexPoints InsertPoint 17 9 2 0.5
  TQhexPoints InsertPoint 18 9 3 0.5
  TQhexPoints InsertPoint 19 8 3 0.5
  TQhexPoints InsertPoint 22 8.5 2 0.5
  TQhexPoints InsertPoint 21 9 2.5 0.5
  TQhexPoints InsertPoint 23 8.5 3 0.5
  TQhexPoints InsertPoint 20 8 2.5 0.5
  TQhexPoints InsertPoint 24 8.5 2.5 0.0
  TQhexPoints InsertPoint 25 8.5 2.5 1
  TQhexPoints InsertPoint 26 8.5 2.5 0.5
vtkFloatArray TQhexScalars
  TQhexScalars SetNumberOfTuples 27
  TQhexScalars InsertValue 0 1.0
  TQhexScalars InsertValue 1 1.0
  TQhexScalars InsertValue 2 1.0
  TQhexScalars InsertValue 3 1.0
  TQhexScalars InsertValue 4 1.0
  TQhexScalars InsertValue 5 1.0
  TQhexScalars InsertValue 6 1.0
  TQhexScalars InsertValue 7 1.0
  TQhexScalars InsertValue 8 0.0
  TQhexScalars InsertValue 9 0.0
  TQhexScalars InsertValue 10 0.0
  TQhexScalars InsertValue 11 0.0
  TQhexScalars InsertValue 12 0.0
  TQhexScalars InsertValue 13 0.0
  TQhexScalars InsertValue 14 0.0
  TQhexScalars InsertValue 15 0.0
  TQhexScalars InsertValue 16 0.0
  TQhexScalars InsertValue 17 0.0
  TQhexScalars InsertValue 18 0.0
  TQhexScalars InsertValue 19 0.0
  TQhexScalars InsertValue 20 0.0
  TQhexScalars InsertValue 21 0.0
  TQhexScalars InsertValue 22 0.0
  TQhexScalars InsertValue 23 0.0
  TQhexScalars InsertValue 24 0.0
  TQhexScalars InsertValue 25 0.0
  TQhexScalars InsertValue 26 0.0
vtkTriQuadraticHexahedron TQHex
  [TQHex GetPointIds] SetId 0 0
  [TQHex GetPointIds] SetId 1 1
  [TQHex GetPointIds] SetId 2 2
  [TQHex GetPointIds] SetId 3 3
  [TQHex GetPointIds] SetId 4 4
  [TQHex GetPointIds] SetId 5 5
  [TQHex GetPointIds] SetId 6 6
  [TQHex GetPointIds] SetId 7 7
  [TQHex GetPointIds] SetId 8 8
  [TQHex GetPointIds] SetId 9 9
  [TQHex GetPointIds] SetId 10 10
  [TQHex GetPointIds] SetId 11 11
  [TQHex GetPointIds] SetId 12 12
  [TQHex GetPointIds] SetId 13 13
  [TQHex GetPointIds] SetId 14 14
  [TQHex GetPointIds] SetId 15 15
  [TQHex GetPointIds] SetId 16 16
  [TQHex GetPointIds] SetId 17 17
  [TQHex GetPointIds] SetId 18 18
  [TQHex GetPointIds] SetId 19 19
  [TQHex GetPointIds] SetId 20 20
  [TQHex GetPointIds] SetId 21 21
  [TQHex GetPointIds] SetId 22 22
  [TQHex GetPointIds] SetId 23 23
  [TQHex GetPointIds] SetId 24 24
  [TQHex GetPointIds] SetId 25 25
  [TQHex GetPointIds] SetId 26 26
vtkUnstructuredGrid TQHexGrid
  TQHexGrid Allocate 1 1
  TQHexGrid InsertNextCell [TQHex GetCellType] [TQHex GetPointIds]
  TQHexGrid SetPoints TQhexPoints
  [TQHexGrid GetPointData] SetScalars TQhexScalars

# BiQuadratic Quadratic hexahedron
vtkPoints BQhexPoints
  BQhexPoints SetNumberOfPoints 24
  BQhexPoints InsertPoint 0 8 4 0
  BQhexPoints InsertPoint 1 9 4 0
  BQhexPoints InsertPoint 2 9 5 0
  BQhexPoints InsertPoint 3 8 5 0
  BQhexPoints InsertPoint 4 8 4 1
  BQhexPoints InsertPoint 5 9 4 1
  BQhexPoints InsertPoint 6 9 5 1
  BQhexPoints InsertPoint 7 8 5 1
  BQhexPoints InsertPoint 8 8.5 4 0
  BQhexPoints InsertPoint 9 9 4.5 0
  BQhexPoints InsertPoint 10 8.5 5 0
  BQhexPoints InsertPoint 11 8 4.5 0
  BQhexPoints InsertPoint 12 8.5 4 1
  BQhexPoints InsertPoint 13 9 4.5 1
  BQhexPoints InsertPoint 14 8.5 5 1
  BQhexPoints InsertPoint 15 8 4.5 1
  BQhexPoints InsertPoint 16 8 4 0.5
  BQhexPoints InsertPoint 17 9 4 0.5
  BQhexPoints InsertPoint 18 9 5 0.5
  BQhexPoints InsertPoint 19 8 5 0.5
  BQhexPoints InsertPoint 22 8.5 4 0.5
  BQhexPoints InsertPoint 21 9 4.5 0.5
  BQhexPoints InsertPoint 23 8.5 5 0.5
  BQhexPoints InsertPoint 20 8 4.5 0.5
vtkFloatArray BQhexScalars
  BQhexScalars SetNumberOfTuples 24
  BQhexScalars InsertValue 0 1.0
  BQhexScalars InsertValue 1 1.0
  BQhexScalars InsertValue 2 1.0
  BQhexScalars InsertValue 3 1.0
  BQhexScalars InsertValue 4 1.0
  BQhexScalars InsertValue 5 1.0
  BQhexScalars InsertValue 6 1.0
  BQhexScalars InsertValue 7 1.0
  BQhexScalars InsertValue 8 0.0
  BQhexScalars InsertValue 9 0.0
  BQhexScalars InsertValue 10 0.0
  BQhexScalars InsertValue 11 0.0
  BQhexScalars InsertValue 12 0.0
  BQhexScalars InsertValue 13 0.0
  BQhexScalars InsertValue 14 0.0
  BQhexScalars InsertValue 15 0.0
  BQhexScalars InsertValue 16 0.0
  BQhexScalars InsertValue 17 0.0
  BQhexScalars InsertValue 18 0.0
  BQhexScalars InsertValue 19 0.0
  BQhexScalars InsertValue 20 0.0
  BQhexScalars InsertValue 21 0.0
  BQhexScalars InsertValue 22 0.0
  BQhexScalars InsertValue 23 0.0
vtkBiQuadraticQuadraticHexahedron BQHex
  [BQHex GetPointIds] SetId 0 0
  [BQHex GetPointIds] SetId 1 1
  [BQHex GetPointIds] SetId 2 2
  [BQHex GetPointIds] SetId 3 3
  [BQHex GetPointIds] SetId 4 4
  [BQHex GetPointIds] SetId 5 5
  [BQHex GetPointIds] SetId 6 6
  [BQHex GetPointIds] SetId 7 7
  [BQHex GetPointIds] SetId 8 8
  [BQHex GetPointIds] SetId 9 9
  [BQHex GetPointIds] SetId 10 10
  [BQHex GetPointIds] SetId 11 11
  [BQHex GetPointIds] SetId 12 12
  [BQHex GetPointIds] SetId 13 13
  [BQHex GetPointIds] SetId 14 14
  [BQHex GetPointIds] SetId 15 15
  [BQHex GetPointIds] SetId 16 16
  [BQHex GetPointIds] SetId 17 17
  [BQHex GetPointIds] SetId 18 18
  [BQHex GetPointIds] SetId 19 19
  [BQHex GetPointIds] SetId 20 20
  [BQHex GetPointIds] SetId 21 21
  [BQHex GetPointIds] SetId 22 22
  [BQHex GetPointIds] SetId 23 23
vtkUnstructuredGrid BQHexGrid
  BQHexGrid Allocate 1 1
  BQHexGrid InsertNextCell [BQHex GetCellType] [BQHex GetPointIds]
  BQHexGrid SetPoints BQhexPoints
  [BQHexGrid GetPointData] SetScalars BQhexScalars



# Quadratic wedge
vtkPoints wedgePoints
  wedgePoints SetNumberOfPoints 15
  wedgePoints InsertPoint 0  10   0   0
  wedgePoints InsertPoint 1  11   0   0
  wedgePoints InsertPoint 2  10   1   0
  wedgePoints InsertPoint 3  10   0   1
  wedgePoints InsertPoint 4  11   0   1
  wedgePoints InsertPoint 5  10   1   1
  wedgePoints InsertPoint 6  10.5 0   0
  wedgePoints InsertPoint 7  10.5 0.5 0
  wedgePoints InsertPoint 8  10   0.5 0
  wedgePoints InsertPoint 9  10.5 0   1
  wedgePoints InsertPoint 10 10.5 0.5 1
  wedgePoints InsertPoint 11 10   0.5 1
  wedgePoints InsertPoint 12 10   0   0.5
  wedgePoints InsertPoint 13 11   0   0.5
  wedgePoints InsertPoint 14 10   1   0.5
vtkFloatArray wedgeScalars
  wedgeScalars SetNumberOfTuples 15
  wedgeScalars InsertValue 0 1.0
  wedgeScalars InsertValue 1 1.0
  wedgeScalars InsertValue 2 1.0
  wedgeScalars InsertValue 3 1.0
  wedgeScalars InsertValue 4 1.0
  wedgeScalars InsertValue 5 1.0
  wedgeScalars InsertValue 6 0.0
  wedgeScalars InsertValue 7 0.0
  wedgeScalars InsertValue 8 0.0
  wedgeScalars InsertValue 9 0.0
  wedgeScalars InsertValue 10 0.0
  wedgeScalars InsertValue 11 0.0
  wedgeScalars InsertValue 12 0.0
  wedgeScalars InsertValue 13 0.0
  wedgeScalars InsertValue 14 0.0
vtkQuadraticWedge aWedge
  [aWedge GetPointIds] SetId 0 0
  [aWedge GetPointIds] SetId 1 1
  [aWedge GetPointIds] SetId 2 2
  [aWedge GetPointIds] SetId 3 3
  [aWedge GetPointIds] SetId 4 4
  [aWedge GetPointIds] SetId 5 5
  [aWedge GetPointIds] SetId 6 6
  [aWedge GetPointIds] SetId 7 7
  [aWedge GetPointIds] SetId 8 8
  [aWedge GetPointIds] SetId 9 9
  [aWedge GetPointIds] SetId 10 10
  [aWedge GetPointIds] SetId 11 11
  [aWedge GetPointIds] SetId 12 12
  [aWedge GetPointIds] SetId 13 13
  [aWedge GetPointIds] SetId 14 14

vtkUnstructuredGrid aWedgeGrid
  aWedgeGrid Allocate 1 1
  aWedgeGrid InsertNextCell [aWedge GetCellType] [aWedge GetPointIds]
  aWedgeGrid SetPoints wedgePoints
  [aWedgeGrid GetPointData] SetScalars wedgeScalars

# Quadratic linear wedge
vtkPoints QLwedgePoints
  QLwedgePoints SetNumberOfPoints 12
  QLwedgePoints InsertPoint 0  10   4   0
  QLwedgePoints InsertPoint 1  11   4   0
  QLwedgePoints InsertPoint 2  10   5   0
  QLwedgePoints InsertPoint 3  10   4   1
  QLwedgePoints InsertPoint 4  11   4   1
  QLwedgePoints InsertPoint 5  10   5   1
  QLwedgePoints InsertPoint 6  10.5 4   0
  QLwedgePoints InsertPoint 7  10.5 4.5 0
  QLwedgePoints InsertPoint 8  10   4.5 0
  QLwedgePoints InsertPoint 9  10.5 4   1
  QLwedgePoints InsertPoint 10 10.5 4.5 1
  QLwedgePoints InsertPoint 11 10   4.5 1
vtkFloatArray QLwedgeScalars
  QLwedgeScalars SetNumberOfTuples 12
  QLwedgeScalars InsertValue 0 1.0
  QLwedgeScalars InsertValue 1 1.0
  QLwedgeScalars InsertValue 2 1.0
  QLwedgeScalars InsertValue 3 1.0
  QLwedgeScalars InsertValue 4 1.0
  QLwedgeScalars InsertValue 5 1.0
  QLwedgeScalars InsertValue 6 0.0
  QLwedgeScalars InsertValue 7 0.0
  QLwedgeScalars InsertValue 8 0.0
  QLwedgeScalars InsertValue 9 0.0
  QLwedgeScalars InsertValue 10 0.0
  QLwedgeScalars InsertValue 11 0.0
vtkQuadraticLinearWedge QLWedge
  [QLWedge GetPointIds] SetId 0 0
  [QLWedge GetPointIds] SetId 1 1
  [QLWedge GetPointIds] SetId 2 2
  [QLWedge GetPointIds] SetId 3 3
  [QLWedge GetPointIds] SetId 4 4
  [QLWedge GetPointIds] SetId 5 5
  [QLWedge GetPointIds] SetId 6 6
  [QLWedge GetPointIds] SetId 7 7
  [QLWedge GetPointIds] SetId 8 8
  [QLWedge GetPointIds] SetId 9 9
  [QLWedge GetPointIds] SetId 10 10
  [QLWedge GetPointIds] SetId 11 11

vtkUnstructuredGrid QLWedgeGrid
  QLWedgeGrid Allocate 1 1
  QLWedgeGrid InsertNextCell [QLWedge GetCellType] [QLWedge GetPointIds]
  QLWedgeGrid SetPoints QLwedgePoints
  [QLWedgeGrid GetPointData] SetScalars QLwedgeScalars

# BiQuadratic wedge
vtkPoints BQwedgePoints
  BQwedgePoints SetNumberOfPoints 18
  BQwedgePoints InsertPoint 0  10   2   0
  BQwedgePoints InsertPoint 1  11   2   0
  BQwedgePoints InsertPoint 2  10   3   0
  BQwedgePoints InsertPoint 3  10   2   1
  BQwedgePoints InsertPoint 4  11   2   1
  BQwedgePoints InsertPoint 5  10   3   1
  BQwedgePoints InsertPoint 6  10.5 2   0
  BQwedgePoints InsertPoint 7  10.5 2.5 0
  BQwedgePoints InsertPoint 8  10   2.5 0
  BQwedgePoints InsertPoint 9  10.5 2   1
  BQwedgePoints InsertPoint 10 10.5 2.5 1
  BQwedgePoints InsertPoint 11 10   2.5 1
  BQwedgePoints InsertPoint 12 10   2   0.5
  BQwedgePoints InsertPoint 13 11   2   0.5
  BQwedgePoints InsertPoint 14 10   3   0.5
  BQwedgePoints InsertPoint 15 10.5 2   0.5
  BQwedgePoints InsertPoint 16 10.5 2.5 0.5
  BQwedgePoints InsertPoint 17 10   2.5 0.5
vtkFloatArray BQwedgeScalars
  BQwedgeScalars SetNumberOfTuples 18
  BQwedgeScalars InsertValue 0 1.0
  BQwedgeScalars InsertValue 1 1.0
  BQwedgeScalars InsertValue 2 1.0
  BQwedgeScalars InsertValue 3 1.0
  BQwedgeScalars InsertValue 4 1.0
  BQwedgeScalars InsertValue 5 1.0
  BQwedgeScalars InsertValue 6 0.0
  BQwedgeScalars InsertValue 7 0.0
  BQwedgeScalars InsertValue 8 0.0
  BQwedgeScalars InsertValue 9 0.0
  BQwedgeScalars InsertValue 10 0.0
  BQwedgeScalars InsertValue 11 0.0
  BQwedgeScalars InsertValue 12 0.0
  BQwedgeScalars InsertValue 13 0.0
  BQwedgeScalars InsertValue 14 0.0
  BQwedgeScalars InsertValue 15 0.0
  BQwedgeScalars InsertValue 16 0.0
  BQwedgeScalars InsertValue 17 0.0
vtkBiQuadraticQuadraticWedge BQWedge
  [BQWedge GetPointIds] SetId 0 0
  [BQWedge GetPointIds] SetId 1 1
  [BQWedge GetPointIds] SetId 2 2
  [BQWedge GetPointIds] SetId 3 3
  [BQWedge GetPointIds] SetId 4 4
  [BQWedge GetPointIds] SetId 5 5
  [BQWedge GetPointIds] SetId 6 6
  [BQWedge GetPointIds] SetId 7 7
  [BQWedge GetPointIds] SetId 8 8
  [BQWedge GetPointIds] SetId 9 9
  [BQWedge GetPointIds] SetId 10 10
  [BQWedge GetPointIds] SetId 11 11
  [BQWedge GetPointIds] SetId 12 12
  [BQWedge GetPointIds] SetId 13 13
  [BQWedge GetPointIds] SetId 14 14
  [BQWedge GetPointIds] SetId 15 15
  [BQWedge GetPointIds] SetId 16 16
  [BQWedge GetPointIds] SetId 17 17

vtkUnstructuredGrid BQWedgeGrid
  BQWedgeGrid Allocate 1 1
  BQWedgeGrid InsertNextCell [BQWedge GetCellType] [BQWedge GetPointIds]
  BQWedgeGrid SetPoints BQwedgePoints
  [BQWedgeGrid GetPointData] SetScalars BQwedgeScalars

# Quadratic pyramid
vtkPoints pyraPoints
  pyraPoints SetNumberOfPoints 13
  pyraPoints InsertPoint 0  12   0   0
  pyraPoints InsertPoint 1  13   0   0
  pyraPoints InsertPoint 2  13   1   0
  pyraPoints InsertPoint 3  12   1   0
  pyraPoints InsertPoint 4  12   0   1
  pyraPoints InsertPoint 5  12.5 0   0
  pyraPoints InsertPoint 6  13   0.5 0
  pyraPoints InsertPoint 7  12.5 1   0
  pyraPoints InsertPoint 8  12   0.5 0
  pyraPoints InsertPoint 9  12   0   0.5
  pyraPoints InsertPoint 10 12.5 0   0.5
  pyraPoints InsertPoint 11 12.5 0.5 0.5
  pyraPoints InsertPoint 12 12   0.5 0.5
vtkFloatArray pyraScalars
  pyraScalars SetNumberOfTuples 13
  pyraScalars InsertValue 0 1.0
  pyraScalars InsertValue 1 1.0
  pyraScalars InsertValue 2 1.0
  pyraScalars InsertValue 3 1.0
  pyraScalars InsertValue 4 1.0
  pyraScalars InsertValue 5 1.0
  pyraScalars InsertValue 6 0.0
  pyraScalars InsertValue 7 0.0
  pyraScalars InsertValue 8 0.0
  pyraScalars InsertValue 9 0.0
  pyraScalars InsertValue 10 0.0
  pyraScalars InsertValue 11 0.0
  pyraScalars InsertValue 12 0.0
vtkQuadraticPyramid aPyramid
  [aPyramid GetPointIds] SetId 0 0
  [aPyramid GetPointIds] SetId 1 1
  [aPyramid GetPointIds] SetId 2 2
  [aPyramid GetPointIds] SetId 3 3
  [aPyramid GetPointIds] SetId 4 4
  [aPyramid GetPointIds] SetId 5 5
  [aPyramid GetPointIds] SetId 6 6
  [aPyramid GetPointIds] SetId 7 7
  [aPyramid GetPointIds] SetId 8 8
  [aPyramid GetPointIds] SetId 9 9
  [aPyramid GetPointIds] SetId 10 10
  [aPyramid GetPointIds] SetId 11 11
  [aPyramid GetPointIds] SetId 12 12

vtkUnstructuredGrid aPyraGrid
  aPyraGrid Allocate 1 1
  aPyraGrid InsertNextCell [aPyramid GetCellType] [aPyramid GetPointIds]
  aPyraGrid SetPoints pyraPoints
  [aPyraGrid GetPointData] SetScalars pyraScalars

# Append the quadratic cells together
vtkAppendFilter appendF
  appendF AddInputData BQuadGrid
  appendF AddInputData QLQuadGrid
  appendF AddInputData QLWedgeGrid
  appendF AddInputData aTriGrid
  appendF AddInputData aQuadGrid
  appendF AddInputData aTetGrid
  appendF AddInputData aHexGrid
  appendF AddInputData TQHexGrid
  appendF AddInputData BQHexGrid
  appendF AddInputData aWedgeGrid
  appendF AddInputData aPyraGrid
  appendF AddInputData BQWedgeGrid

# Extract the edges
vtkExtractEdges extract
  extract SetInputConnection [appendF GetOutputPort]

vtkShrinkPolyData shrink
  shrink SetInputConnection [extract GetOutputPort]
  shrink SetShrinkFactor 0.90

vtkDataSetMapper aMapper
  aMapper SetInputConnection [shrink GetOutputPort]
  #aMapper ScalarVisibilityOff

vtkActor aActor
  aActor SetMapper aMapper
  [aActor GetProperty] SetRepresentationToWireframe

# Create the rendering related stuff.
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

ren1 SetBackground .1 .2 .3
renWin SetSize 400 200

# specify properties
ren1 AddActor aActor
renWin Render
[ren1 GetActiveCamera] Dolly 2.0
ren1 ResetCameraClippingRange

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .


