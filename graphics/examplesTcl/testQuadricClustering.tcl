catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Generate implicit model of a sphere
#
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create renderer stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# pipeline stuff
#

vtkSphereSource sphere
  sphere SetPhiResolution 150
  sphere SetThetaResolution 150

vtkPoints pts
  pts InsertNextPoint 0 0 0
  pts InsertNextPoint 1 0 0
  pts InsertNextPoint 0 1 0
  pts InsertNextPoint 0 0 1


vtkCellArray tris
  tris InsertNextCell 3
  tris InsertCellPoint 0
  tris InsertCellPoint 1
  tris InsertCellPoint 2
  tris InsertNextCell 3
  tris InsertCellPoint 0
  tris InsertCellPoint 2
  tris InsertCellPoint 3
  tris InsertNextCell 3
  tris InsertCellPoint 0
  tris InsertCellPoint 3
  tris InsertCellPoint 1
  tris InsertNextCell 3
  tris InsertCellPoint 1
  tris InsertCellPoint 2
  tris InsertCellPoint 3

vtkPolyData polys
  polys SetPoints pts
  polys SetPolys tris

vtkQuadricClustering mesh
  mesh SetInput [sphere GetOutput]
#  mesh SetInput polys
  mesh SetNumberOfXDivisions 20
  mesh SetNumberOfYDivisions 20
  mesh SetNumberOfZDivisions 20

vtkPolyDataMapper mapper
  mapper SetInput [mesh GetOutput]
vtkActor actor
  actor SetMapper mapper

# just for debugging
vtkPolyDataMapper mapper2
  mapper2 SetInput polys
vtkActor actor2
  actor2 SetMapper mapper2
  [actor2 GetProperty] SetColor 1.0 0.5 0.5
#ren1 AddActor actor2

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}


# prevent the tk window from showing up then start the event loop
wm withdraw .