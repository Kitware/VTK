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
  triScalars InsertValue 4 0.0
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

# Append the quadratic cells together
vtkAppendFilter appendF
  appendF AddInput aTriGrid
  appendF AddInput aQuadGrid
  appendF AddInput aTetGrid
  appendF AddInput aHexGrid

# Extract the edges
vtkExtractEdges extract
  extract SetInput [appendF GetOutput]

vtkShrinkPolyData shrink
  shrink SetInput [extract GetOutput]
  shrink SetShrinkFactor 0.90

vtkDataSetMapper aMapper
  aMapper SetInput [shrink GetOutput]
  aMapper ScalarVisibilityOff

vtkActor aActor
  aActor SetMapper aMapper
  [aActor GetProperty] SetRepresentationToWireframe
  [aActor GetProperty] SetAmbient 1.0

# Create the rendering related stuff.
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

ren1 SetBackground .1 .2 .3
renWin SetSize 400 150

# specify properties
ren1 AddActor aActor 
renWin Render
[ren1 GetActiveCamera] Dolly 3.0
ren1 ResetCameraClippingRange

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .


