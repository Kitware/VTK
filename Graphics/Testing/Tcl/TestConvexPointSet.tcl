package require vtk
package require vtkinteraction

# create points in the configuration of an octant with one 2:1 face
#
vtkPoints points
vtkConvexPointSet aConvex
points InsertPoint 0 0 0 0
points InsertPoint 1 1 0 0
points InsertPoint 2 1 1 0
points InsertPoint 3 0 1 0
points InsertPoint 4 0 0 1
points InsertPoint 5 1 0 1
points InsertPoint 6 1 1 1
points InsertPoint 7 0 1 1
points InsertPoint 8 0.5 0 0
points InsertPoint 9 1 0.5 0
points InsertPoint 10 0.5 1 0
points InsertPoint 11 0 0.5 0
points InsertPoint 12 0.5 0.5 0
for {set i 0} {$i<13} {incr i 1} {
    [aConvex GetPointIds] InsertId $i $i
}

vtkUnstructuredGrid aConvexGrid
  aConvexGrid Allocate 1 1
  aConvexGrid InsertNextCell [aConvex GetCellType] [aConvex GetPointIds]
  aConvexGrid SetPoints points

# Display the cell
vtkDataSetMapper dsm
  dsm SetInput aConvexGrid
vtkActor a
  a SetMapper dsm
  eval [a GetProperty] SetColor 0 1 0

# Contour and clip the cell with elevation scalars
vtkElevationFilter ele
  ele SetInput aConvexGrid
  ele SetLowPoint -1 -1 -1
  ele SetHighPoint 1 1 1
  ele SetScalarRange -1 1
    
# Clip
#
vtkClipDataSet clip
  clip SetInputConnection [ele GetOutputPort]
  clip SetValue 0.5
vtkDataSetSurfaceFilter g
  g SetInputConnection [clip GetOutputPort]
vtkPolyDataMapper map
  map SetInputConnection [g GetOutputPort]
  map ScalarVisibilityOff
vtkActor clipActor
  clipActor SetMapper map
  [clipActor GetProperty] SetColor 1 0 0
  clipActor AddPosition 2 0 0

# Contour
#
vtkContourFilter contour
  contour SetInputConnection [ele GetOutputPort]
  contour SetValue 0 0.5
vtkDataSetSurfaceFilter g2
  g2 SetInputConnection [contour GetOutputPort]
vtkPolyDataMapper map2
  map2 SetInputConnection [g2 GetOutputPort]
  map2 ScalarVisibilityOff
vtkActor contourActor
  contourActor SetMapper map2
  [contourActor GetProperty] SetColor 1 0 0
  contourActor AddPosition 1 2 0

# Create graphics stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor a
ren1 AddActor clipActor
ren1 AddActor contourActor

ren1 SetBackground 1 1 1
renWin SetSize 250 150

vtkCamera aCam
  aCam SetFocalPoint 1.38705 1.37031 0.639901
  aCam SetPosition 1.89458 -5.07106 -4.17439
  aCam SetViewUp 0.00355726 0.598843 -0.800858
  aCam SetClippingRange 4.82121 12.1805
ren1 SetActiveCamera aCam

renWin Render

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.5

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .



