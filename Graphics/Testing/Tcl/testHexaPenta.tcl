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
  clip SetInput [reader GetOutput]
  clip SetClipFunction plane
  clip GenerateClipScalarsOn
vtkDataSetSurfaceFilter g
  g SetInput [clip GetOutput]
vtkPolyDataMapper map
  map SetInput [g GetOutput]
vtkActor clipActor
  clipActor SetMapper map

# Contour
#
vtkContourFilter contour
  contour SetInput [reader GetOutput]
  contour SetValue 0 0.125
  contour SetValue 1 0.25
  contour SetValue 2 0.5
  contour SetValue 3 0.75
  contour SetValue 4 1.0
vtkDataSetSurfaceFilter g2
  g2 SetInput [contour GetOutput]
vtkPolyDataMapper map2
  map2 SetInput [g2 GetOutput]
  map2 ScalarVisibilityOff
vtkActor contourActor
  contourActor SetMapper map2
  [contourActor GetProperty] SetColor 1 0 0
  [contourActor GetProperty] SetRepresentationToWireframe
  
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
ren1 AddActor contourActor

ren1 SetBackground 1 1 1

renWin Render

# render the image
#
iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .



