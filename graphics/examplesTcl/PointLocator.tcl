catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

vtkPointSource ps
ps SetNumberOfPoints 2000
ps Update

vtkPolyDataMapper map
map SetInput [ps GetOutput]

vtkActor points
points SetMapper map

# Create graphics stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
#ren1 AddActor points
#renWin Render
iren SetUserMethod {wm deiconify .vtkInteract}

vtkSphereSource ss
ss SetRadius 0.02

vtkPolyData pd
vtkPoints pts
pd SetPoints pts

vtkGlyph3D gly
gly SetSource [ss GetOutput]
gly SetInput pd

vtkPolyDataMapper ssmap
ssmap SetInput [gly GetOutput]

vtkActor ssact
ssact SetMapper ssmap
[ssact GetProperty] SetColor 1 0.3 1

vtkPolyDataMapper bmap
bmap SetInput [ss GetOutput]

vtkActor ba
ba SetMapper bmap
[ba GetProperty] SetColor 0.5 1 0.5
ba SetScale 1.1 1.1 1.1

ren1 AddActor ssact
ren1 AddActor ba

vtkPointLocator2D pl
pl SetDataSet [ps GetOutput]

vtkIdList idl

set loc {0.3 0.3 0.3}
eval ba SetPosition $loc
eval pl FindPointsWithinRadius 0.1 [lindex $loc 0] [lindex $loc 1] idl
pts Reset
for {set i 0} {$i < [idl GetNumberOfIds]} {incr i} {
    eval pts InsertNextPoint [[[ps GetOutput] GetPoints] GetPoint [idl GetId $i]]
}
pd Modified
[ren1 GetActiveCamera] Zoom 1.7
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .



