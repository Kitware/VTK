package require vtk
package require vtkinteraction
package require vtktesting

#
# Demonstrate the use of clipping and capping on polyhedral data
#

# create a sphere and clip it
#
vtkSphereSource sphere
    sphere SetRadius 1
    sphere SetPhiResolution 10
    sphere SetThetaResolution 10
vtkPlane plane
    plane SetOrigin 0 0 0
    plane SetNormal -1 -1 0
vtkClipPolyData clipper
    clipper SetInputConnection [sphere GetOutputPort]
    clipper SetClipFunction plane
    clipper GenerateClipScalarsOn
    clipper GenerateClippedOutputOn
    clipper SetValue 0
vtkPolyDataMapper clipMapper
    clipMapper SetInputConnection [clipper GetOutputPort]
    clipMapper ScalarVisibilityOff

vtkProperty backProp
    eval backProp SetDiffuseColor $tomato
vtkActor clipActor
    clipActor SetMapper clipMapper
    eval [clipActor GetProperty] SetColor $peacock
    clipActor SetBackfaceProperty backProp

# now extract feature edges
vtkFeatureEdges boundaryEdges
  boundaryEdges SetInputConnection [clipper GetOutputPort]
  boundaryEdges BoundaryEdgesOn
  boundaryEdges FeatureEdgesOff
  boundaryEdges NonManifoldEdgesOff

vtkCleanPolyData boundaryClean
  boundaryClean SetInputConnection [boundaryEdges GetOutputPort]

vtkStripper boundaryStrips
  boundaryStrips SetInputConnection [boundaryClean GetOutputPort]
  boundaryStrips Update

vtkPolyData boundaryPoly
  boundaryPoly SetPoints [[boundaryStrips GetOutput] GetPoints]
  boundaryPoly SetPolys [[boundaryStrips GetOutput] GetLines]

vtkTriangleFilter boundaryTriangles
  boundaryTriangles SetInput boundaryPoly

vtkPolyDataMapper boundaryMapper
boundaryMapper SetInputConnection [boundaryTriangles GetOutputPort]

vtkActor boundaryActor
  boundaryActor SetMapper boundaryMapper
  eval [boundaryActor GetProperty] SetColor $banana

# Create graphics stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor clipActor
ren1 AddActor boundaryActor
ren1 SetBackground 1 1 1
ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 30
[ren1 GetActiveCamera] Dolly 1.2
ren1 ResetCameraClippingRange

renWin SetSize 300 300
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
