#
# Draws a plane with texture and a 2D triangle.
# Test to see if texture is disabled properly in 2D Mapper
#
# Derived from a bug reported by Robert Stein, NCSA
#

catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

#
# get the interactor ui

source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow  Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# load the texture map
vtkPNMReader pnm
  pnm SetFileName "$VTK_DATA/masonry.ppm"

vtkTexture atext
  atext SetInput [pnm GetOutput   ]
  atext InterpolateOn  

vtkPlaneSource plane
vtkPolyDataMapper planeMapper
  planeMapper SetInput [plane GetOutput   ]
vtkActor planeActor
  planeActor SetMapper planeMapper 
  planeActor SetTexture atext 

ren1 SetBackground 0.2 0.3 0.4 
renWin SetSize 300 300 

vtkCellArray cells
  cells InsertNextCell 3
  cells InsertCellPoint 0
  cells InsertCellPoint 1
  cells InsertCellPoint 2
vtkPoints points
  points InsertPoint 0  -20  -20  0 
  points InsertPoint 1  20  -20  0 
  points InsertPoint 2  0  20  0 

vtkPolyData camera
  camera SetPoints points 
  camera SetPolys cells 
  
vtkPolyDataMapper2D cameraMapper
  cameraMapper SetInput camera 
vtkActor2D cameraActor
  cameraActor SetMapper cameraMapper 
  cameraActor SetPosition 100  100 

ren1 AddActor cameraActor 
ren1 AddActor planeActor 
  
iren Initialize

renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
