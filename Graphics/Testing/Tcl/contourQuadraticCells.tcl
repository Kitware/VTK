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
vtkContourFilter edgeContours
  edgeContours SetInput aEdgeGrid
  edgeContours SetValue 0 0.5
vtkDataSetMapper aEdgeContourMapper
  aEdgeContourMapper SetInput [edgeContours GetOutput]
  aEdgeContourMapper ScalarVisibilityOff
vtkDataSetMapper aEdgeMapper
  aEdgeMapper SetInput aEdgeGrid
  aEdgeMapper ScalarVisibilityOff
vtkActor aEdgeActor
  aEdgeActor SetMapper aEdgeMapper
  [aEdgeActor GetProperty] SetRepresentationToWireframe
vtkActor aEdgeContourActor
  aEdgeContourActor SetMapper aEdgeContourMapper
  [aEdgeContourActor GetProperty] BackfaceCullingOn

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
vtkContourFilter triContours
  triContours SetInput aTriGrid
  triContours SetValue 0 0.5
vtkDataSetMapper aTriContourMapper
  aTriContourMapper SetInput [triContours GetOutput]
  aTriContourMapper ScalarVisibilityOff
vtkDataSetMapper aTriMapper
  aTriMapper SetInput aTriGrid
  aTriMapper ScalarVisibilityOff
vtkActor aTriActor
  aTriActor SetMapper aTriMapper
  [aTriActor GetProperty] SetRepresentationToWireframe
vtkActor aTriContourActor
  aTriContourActor SetMapper aTriContourMapper
  [aTriContourActor GetProperty] BackfaceCullingOn

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
  quadScalars InsertValue 2 0.0
  quadScalars InsertValue 3 0.0
  quadScalars InsertValue 4 1.0
  quadScalars InsertValue 5 0.0
  quadScalars InsertValue 6 1.0
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
vtkContourFilter quadContours
  quadContours SetInput aQuadGrid
  quadContours SetValue 0 0.5
vtkDataSetMapper aQuadContourMapper
  aQuadContourMapper SetInput [quadContours GetOutput]
  aQuadContourMapper ScalarVisibilityOff
vtkDataSetMapper aQuadMapper
  aQuadMapper SetInput aQuadGrid
  aQuadMapper ScalarVisibilityOff
vtkActor aQuadActor
  aQuadActor SetMapper aQuadMapper
  [aQuadActor GetProperty] SetRepresentationToWireframe
vtkActor aQuadContourActor
  aQuadContourActor SetMapper aQuadContourMapper
  [aQuadContourActor GetProperty] BackfaceCullingOn

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
renWin SetSize 400 400

# specify properties
ren1 AddActor aEdgeActor 
[aEdgeActor GetProperty] SetDiffuseColor 1 0 0
ren1 AddActor aEdgeContourActor
[aEdgeContourActor GetProperty] SetDiffuseColor 1 0 0

ren1 AddActor aTriActor 
[aTriActor GetProperty] SetDiffuseColor 1 0 0
ren1 AddActor aTriContourActor
[aTriContourActor GetProperty] SetDiffuseColor 1 0 0

ren1 AddActor aQuadActor 
[aQuadActor GetProperty] SetDiffuseColor 1 0 0
ren1 AddActor aQuadContourActor
[aQuadContourActor GetProperty] SetDiffuseColor 1 0 0

# places everyone!!
aEdgeContourActor AddPosition 0 2 0
aTriActor AddPosition 2 0 0
aTriContourActor AddPosition 2 2 0
aQuadActor AddPosition 4 0 0
aQuadContourActor AddPosition 4 2 0

BuildBackdrop -1 11 -1 16 -1 2 .1

ren1 AddActor base
[base GetProperty] SetDiffuseColor .2 .2 .2
ren1 AddActor left
[left GetProperty] SetDiffuseColor .2 .2 .2
ren1 AddActor back
[back GetProperty] SetDiffuseColor .2 .2 .2

[ren1 GetActiveCamera] Dolly 1.5
ren1 ResetCameraClippingRange

renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .


