package require vtk
package require vtkinteraction

# Color points with scalars

# get the interactor ui

vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# create some points with scalars

vtkUnsignedCharArray chars
  chars SetNumberOfComponents 3
  chars SetNumberOfTuples 3
  chars InsertComponent 0 0 255
  chars InsertComponent 0 1 99
  chars InsertComponent 0 2 71

  chars InsertComponent 1 0 125
  chars InsertComponent 1 1 255
  chars InsertComponent 1 2 0

  chars InsertComponent 2 0 226
  chars InsertComponent 2 1 207
  chars InsertComponent 2 2 87


vtkPoints polyVertexPoints
  polyVertexPoints SetNumberOfPoints 3
  polyVertexPoints InsertPoint 0 0 0 0
  polyVertexPoints InsertPoint 1 1 0 0
  polyVertexPoints InsertPoint 2 1 1 0

vtkPolyVertex aPolyVertex
  [aPolyVertex GetPointIds] SetNumberOfIds 3
  [aPolyVertex GetPointIds] SetId 0 0
  [aPolyVertex GetPointIds] SetId 1 1
  [aPolyVertex GetPointIds] SetId 2 2

vtkUnstructuredGrid aPolyVertexGrid
  aPolyVertexGrid Allocate 1 1
  aPolyVertexGrid InsertNextCell [aPolyVertex GetCellType] [aPolyVertex GetPointIds]
  aPolyVertexGrid SetPoints polyVertexPoints
  [aPolyVertexGrid GetPointData] SetScalars chars

vtkSphereSource sphere
  sphere SetRadius .1

vtkGlyph3D glyphs
  glyphs ScalingOff
  glyphs SetColorModeToColorByScalar
  glyphs SetScaleModeToDataScalingOff
  glyphs SetInput aPolyVertexGrid
  glyphs SetSource [sphere GetOutput]

vtkDataSetMapper glyphsMapper
  glyphsMapper SetInput [glyphs GetOutput]

vtkActor glyphsActor
  glyphsActor SetMapper glyphsMapper
  [glyphsActor GetProperty] BackfaceCullingOn


ren1 SetBackground .1 .2 .4

ren1 AddActor glyphsActor; [glyphsActor GetProperty] SetDiffuseColor 1 1 1

[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 20
[ren1 GetActiveCamera] Dolly 1.25
ren1 ResetCameraClippingRange

renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .


