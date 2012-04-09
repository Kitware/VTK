package require vtk
package require vtkinteraction
package require vtktesting

# Contour every quadratic cell type

# Create a scene with one of each cell type.
# QuadraticEdge
vtkPoints edgePoints
  edgePoints SetNumberOfPoints 3
  edgePoints InsertPoint 0 0 0 0
  edgePoints InsertPoint 1 1.0 0 0
  edgePoints InsertPoint 2 0.5 0.25 0
vtkFloatArray edgeScalars
  edgeScalars SetNumberOfTuples 3
  edgeScalars InsertValue 0 0.0
  edgeScalars InsertValue 1 0.0
  edgeScalars InsertValue 2 0.9
vtkQuadraticEdge aEdge
  [aEdge GetPointIds] SetId 0 0
  [aEdge GetPointIds] SetId 1 1
  [aEdge GetPointIds] SetId 2 2
vtkUnstructuredGrid aEdgeGrid
  aEdgeGrid Allocate 1 1
  aEdgeGrid InsertNextCell [aEdge GetCellType] [aEdge GetPointIds]
  aEdgeGrid SetPoints edgePoints
  [aEdgeGrid GetPointData] SetScalars edgeScalars
vtkDataSetMapper aEdgeMapper
  aEdgeMapper SetInputData aEdgeGrid
  aEdgeMapper ScalarVisibilityOff
vtkActor aEdgeActor
  aEdgeActor SetMapper aEdgeMapper
  [aEdgeActor GetProperty] SetRepresentationToWireframe
  [aEdgeActor GetProperty] SetAmbient 1.0

# Quadratic triangle
vtkPoints triPoints
  triPoints SetNumberOfPoints 6
  triPoints InsertPoint 0 0.0 0.0 0.0
  triPoints InsertPoint 1 1.0 0.0 0.0
  triPoints InsertPoint 2 0.5 0.8 0.0
  triPoints InsertPoint 3 0.5 0.0 0.0
  triPoints InsertPoint 4 0.75 0.4 0.0
  triPoints InsertPoint 5 0.25 0.4 0.0
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
vtkDataSetMapper aTriMapper
  aTriMapper SetInputData aTriGrid
  aTriMapper ScalarVisibilityOff
vtkActor aTriActor
  aTriActor SetMapper aTriMapper
  [aTriActor GetProperty] SetRepresentationToWireframe
  [aTriActor GetProperty] SetAmbient 1.0

# Quadratic quadrilateral
vtkPoints quadPoints
  quadPoints SetNumberOfPoints 8
  quadPoints InsertPoint 0 0.0 0.0 0.0
  quadPoints InsertPoint 1 1.0 0.0 0.0
  quadPoints InsertPoint 2 1.0 1.0 0.0
  quadPoints InsertPoint 3 0.0 1.0 0.0
  quadPoints InsertPoint 4 0.5 0.0 0.0
  quadPoints InsertPoint 5 1.0 0.5 0.0
  quadPoints InsertPoint 6 0.5 1.0 0.0
  quadPoints InsertPoint 7 0.0 0.5 0.0
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
vtkDataSetMapper aQuadMapper
  aQuadMapper SetInputData aQuadGrid
  aQuadMapper ScalarVisibilityOff
vtkActor aQuadActor
  aQuadActor SetMapper aQuadMapper
  [aQuadActor GetProperty] SetRepresentationToWireframe
  [aQuadActor GetProperty] SetAmbient 1.0

# Quadratic tetrahedron
vtkPoints tetPoints
  tetPoints SetNumberOfPoints 10
  tetPoints InsertPoint 0 0.0 0.0 0.0
  tetPoints InsertPoint 1 1.0 0.0 0.0
  tetPoints InsertPoint 2 0.5 0.8 0.0
  tetPoints InsertPoint 3 0.5 0.4 1.0
  tetPoints InsertPoint 4 0.5 0.0 0.0
  tetPoints InsertPoint 5 0.75 0.4 0.0
  tetPoints InsertPoint 6 0.25 0.4 0.0
  tetPoints InsertPoint 7 0.25 0.2 0.5
  tetPoints InsertPoint 8 0.75 0.2 0.5
  tetPoints InsertPoint 9 0.50 0.6 0.5
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
vtkDataSetMapper aTetMapper
  aTetMapper SetInputData aTetGrid
  aTetMapper ScalarVisibilityOff
vtkActor aTetActor
  aTetActor SetMapper aTetMapper
  [aTetActor GetProperty] SetRepresentationToWireframe
  [aTetActor GetProperty] SetAmbient 1.0

# Quadratic hexahedron
vtkPoints hexPoints
  hexPoints SetNumberOfPoints 20
  hexPoints InsertPoint 0 0 0 0
  hexPoints InsertPoint 1 1 0 0
  hexPoints InsertPoint 2 1 1 0
  hexPoints InsertPoint 3 0 1 0
  hexPoints InsertPoint 4 0 0 1
  hexPoints InsertPoint 5 1 0 1
  hexPoints InsertPoint 6 1 1 1
  hexPoints InsertPoint 7 0 1 1
  hexPoints InsertPoint 8 0.5 0 0
  hexPoints InsertPoint 9 1 0.5 0
  hexPoints InsertPoint 10 0.5 1 0
  hexPoints InsertPoint 11 0 0.5 0
  hexPoints InsertPoint 12 0.5 0 1
  hexPoints InsertPoint 13 1 0.5 1
  hexPoints InsertPoint 14 0.5 1 1
  hexPoints InsertPoint 15 0 0.5 1
  hexPoints InsertPoint 16 0 0 0.5
  hexPoints InsertPoint 17 1 0 0.5
  hexPoints InsertPoint 18 1 1 0.5
  hexPoints InsertPoint 19 0 1 0.5
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
vtkDataSetMapper aHexMapper
  aHexMapper SetInputData aHexGrid
  aHexMapper ScalarVisibilityOff
vtkActor aHexActor
  aHexActor SetMapper aHexMapper
  [aHexActor GetProperty] SetRepresentationToWireframe
  [aHexActor GetProperty] SetAmbient 1.0

# Quadratic wedge
vtkPoints wedgePoints
  wedgePoints SetNumberOfPoints 15
  wedgePoints InsertPoint 0   0   0   0
  wedgePoints InsertPoint 1   1   0   0
  wedgePoints InsertPoint 2   0   1   0
  wedgePoints InsertPoint 3   0   0   1
  wedgePoints InsertPoint 4   1   0   1
  wedgePoints InsertPoint 5   0   1   1
  wedgePoints InsertPoint 6   0.5 0   0
  wedgePoints InsertPoint 7   0.5 0.5 0
  wedgePoints InsertPoint 8   0   0.5 0
  wedgePoints InsertPoint 9   0.5 0   1
  wedgePoints InsertPoint 10  0.5 0.5 1
  wedgePoints InsertPoint 11  0   0.5 1
  wedgePoints InsertPoint 12  0   0   0.5
  wedgePoints InsertPoint 13  1   0   0.5
  wedgePoints InsertPoint 14  0   1   0.5
vtkFloatArray wedgeScalars
  wedgeScalars SetNumberOfTuples 15
  wedgeScalars InsertValue 0 1.0
  wedgeScalars InsertValue 1 1.0
  wedgeScalars InsertValue 2 1.0
  wedgeScalars InsertValue 3 1.0
  wedgeScalars InsertValue 4 1.0
  wedgeScalars InsertValue 5 1.0
  wedgeScalars InsertValue 6 1.0
  wedgeScalars InsertValue 7 1.0
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
vtkClipDataSet wedgeContours
  wedgeContours SetInputData aWedgeGrid
  wedgeContours SetValue 0.5
vtkDataSetMapper aWedgeContourMapper
  aWedgeContourMapper SetInputConnection [wedgeContours GetOutputPort]
  aWedgeContourMapper ScalarVisibilityOff
vtkDataSetMapper aWedgeMapper
  aWedgeMapper SetInputData aWedgeGrid
  aWedgeMapper ScalarVisibilityOff
vtkActor aWedgeActor
  aWedgeActor SetMapper aWedgeMapper
  [aWedgeActor GetProperty] SetRepresentationToWireframe
  [aWedgeActor GetProperty] SetAmbient 1.0
vtkActor aWedgeContourActor
  aWedgeContourActor SetMapper aWedgeContourMapper
  [aWedgeContourActor GetProperty] SetAmbient 1.0

# Quadratic pyramid
vtkPoints pyraPoints
  pyraPoints SetNumberOfPoints 13
  pyraPoints InsertPoint 0  0   0   0
  pyraPoints InsertPoint 1  1   0   0
  pyraPoints InsertPoint 2  1   1   0
  pyraPoints InsertPoint 3  0   1   0
  pyraPoints InsertPoint 4  0   0   1
  pyraPoints InsertPoint 5  0.5 0   0
  pyraPoints InsertPoint 6  1   0.5 0
  pyraPoints InsertPoint 7  0.5 1   0
  pyraPoints InsertPoint 8  0   0.5 0
  pyraPoints InsertPoint 9  0   0   0.5
  pyraPoints InsertPoint 10 0.5 0   0.5
  pyraPoints InsertPoint 11 0.5 0.5 0.5
  pyraPoints InsertPoint 12 0   0.5 0.5
vtkFloatArray pyraScalars
  pyraScalars SetNumberOfTuples 13
  pyraScalars InsertValue 0 1.0
  pyraScalars InsertValue 1 1.0
  pyraScalars InsertValue 2 1.0
  pyraScalars InsertValue 3 1.0
  pyraScalars InsertValue 4 1.0
  pyraScalars InsertValue 5 1.0
  pyraScalars InsertValue 6 1.0
  pyraScalars InsertValue 7 1.0
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
vtkUnstructuredGrid aPyramidGrid
  aPyramidGrid Allocate 1 1
  aPyramidGrid InsertNextCell [aPyramid GetCellType] [aPyramid GetPointIds]
  aPyramidGrid SetPoints pyraPoints
  [aPyramidGrid GetPointData] SetScalars pyraScalars
vtkClipDataSet pyraContours
  pyraContours SetInputData aPyramidGrid
  pyraContours SetValue 0.5
vtkDataSetMapper aPyramidContourMapper
  aPyramidContourMapper SetInputConnection [pyraContours GetOutputPort]
  aPyramidContourMapper ScalarVisibilityOff
vtkDataSetMapper aPyramidMapper
  aPyramidMapper SetInputData aPyramidGrid
  aPyramidMapper ScalarVisibilityOff
vtkActor aPyramidActor
  aPyramidActor SetMapper aPyramidMapper
  [aPyramidActor GetProperty] SetRepresentationToWireframe
  [aPyramidActor GetProperty] SetAmbient 1.0
vtkActor aPyramidContourActor
  aPyramidContourActor SetMapper aPyramidContourMapper
  [aPyramidContourActor GetProperty] SetAmbient 1.0

# Create the rendering related stuff.
# Since some of our actors are a single vertex, we need to remove all
# cullers so the single vertex actors will render
vtkRenderer ren1
[ren1 GetCullers] RemoveAllItems

vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

ren1 SetBackground .1 .2 .3
renWin SetSize 400 200

# specify properties
ren1 AddActor aEdgeActor
ren1 AddActor aTriActor
ren1 AddActor aQuadActor
ren1 AddActor aTetActor
ren1 AddActor aHexActor
ren1 AddActor aWedgeActor
ren1 AddActor aPyramidActor

# places everyone!!
aTriActor AddPosition 2 0 0
aQuadActor AddPosition 4 0 0
aTetActor AddPosition 6 0 0
aHexActor AddPosition 8 0 0
aWedgeActor AddPosition 10 0 0
aPyramidActor AddPosition 12 0 0

BuildBackdrop -1 15 -1 4 -1 2 .1
ren1 AddActor base
[base GetProperty] SetDiffuseColor .2 .2 .2
ren1 AddActor left
[left GetProperty] SetDiffuseColor .2 .2 .2
ren1 AddActor back
[back GetProperty] SetDiffuseColor .2 .2 .2

ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 2.5
ren1 ResetCameraClippingRange

renWin Render

# create a little scorecard above each of the cells. These are displayed
# if a ray cast hits the cell, otherwise they are not shown.
vtkPlaneSource pm
  pm SetXResolution 1
  pm SetYResolution 1
vtkPolyDataMapper pmapper
  pmapper SetInputConnection [pm GetOutputPort]

# now try intersecting rays with the cell
vtkCellPicker cellPicker

vtkActor edgeCheck
  edgeCheck SetMapper pmapper
  edgeCheck AddPosition 0.5 2.5 0
cellPicker Pick 87 71 0 ren1
if {[cellPicker GetCellId] != "-1" } {
    ren1 AddActor edgeCheck
}

vtkActor triCheck
  triCheck SetMapper pmapper
  triCheck AddPosition 2.5 2.5 0
cellPicker Pick 139 72 0 ren1
if {[cellPicker GetCellId] != "-1" } {
    ren1 AddActor triCheck
}

vtkActor quadCheck
  quadCheck SetMapper pmapper
  quadCheck AddPosition 4.5 2.5 0
cellPicker Pick 192 78 0 ren1
if {[cellPicker GetCellId] != "-1" } {
    ren1 AddActor quadCheck
}

vtkActor tetCheck
  tetCheck SetMapper pmapper
  tetCheck AddPosition 6.5 2.5 0
cellPicker Pick 233 70 0 ren1
if {[cellPicker GetCellId] != "-1" } {
    ren1 AddActor tetCheck
}

vtkActor hexCheck
  hexCheck SetMapper pmapper
  hexCheck AddPosition 8.5 2.5 0
cellPicker Pick 287 80 0 ren1
if {[cellPicker GetCellId] != "-1" } {
    ren1 AddActor hexCheck
}

vtkActor wedgeCheck
  wedgeCheck SetMapper pmapper
  wedgeCheck AddPosition 10.5 2.5 0
cellPicker Pick 287 80 0 ren1
if {[cellPicker GetCellId] != "-1" } {
    ren1 AddActor wedgeCheck
}

vtkActor pyraCheck
  pyraCheck SetMapper pmapper
  pyraCheck AddPosition 12.5 2.5 0
cellPicker Pick 287 80 0 ren1
if {[cellPicker GetCellId] != "-1" } {
    ren1 AddActor pyraCheck
}

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .


