package require vtk
package require vtkinteraction

# read the football dataset:
#

vtkUnstructuredGridReader reader
  reader SetFileName "$VTK_DATA_ROOT/Data/PentaHexa.vtk"
  reader Update

# Clip
#
vtkPlane plane
    plane SetNormal 1 1 0
vtkClipDataSet clip
  clip SetInputConnection [reader GetOutputPort]
  clip SetClipFunction plane
  clip GenerateClipScalarsOn
vtkDataSetSurfaceFilter g
  g SetInputConnection [clip GetOutputPort]
vtkPolyDataMapper map
  map SetInputConnection [g GetOutputPort]
vtkActor clipActor
  clipActor SetMapper map

# Contour
#
vtkContourFilter contour
  contour SetInputConnection [reader GetOutputPort]
  contour SetValue 0 0.125
  contour SetValue 1 0.25
  contour SetValue 2 0.5
  contour SetValue 3 0.75
  contour SetValue 4 1.0
vtkDataSetSurfaceFilter g2
  g2 SetInputConnection [contour GetOutputPort]
vtkPolyDataMapper map2
  map2 SetInputConnection [g2 GetOutputPort]
  map2 ScalarVisibilityOff
vtkActor contourActor
  contourActor SetMapper map2
  [contourActor GetProperty] SetColor 1 0 0
  [contourActor GetProperty] SetRepresentationToWireframe


# Triangulate
vtkDataSetTriangleFilter tris
  tris SetInputConnection [reader GetOutputPort]

vtkShrinkFilter shrink
  shrink SetInputConnection [tris GetOutputPort]
  shrink SetShrinkFactor .8

vtkDataSetMapper map3
  map3 SetInputConnection [shrink GetOutputPort]
  map3 SetScalarRange 0 26

vtkActor triActor
  triActor SetMapper map3
  triActor AddPosition 2 0 0 
  
# Create graphics stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin SetMultiSamples 0
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor clipActor
ren1 AddActor contourActor
ren1 AddActor triActor

ren1 SetBackground 1 1 1

renWin Render

# render the image
#
iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .



