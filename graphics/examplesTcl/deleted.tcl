catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# an example of deleting a rendering window
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1

vtkSphereSource ls
vtkPolyDataMapper pdm
pdm SetInput [ls GetOutput]
vtkActor a1
a1 SetMapper pdm
ren1 AddActor a1
renWin SetSize 400 400
[a1 GetProperty] SetColor 0.6 0.4 1.0
ren1 SetBackground 0.5 0.7 0.3
renWin Render

# delete the old ones
rename renWin {}
rename ren1 {}

# create a new window
vtkRenderWindow renWin
vtkRenderer ren1
   renWin AddRenderer ren1

ren1 AddActor a1
renWin SetSize 300 300
[a1 GetProperty] SetColor 0.4 0.6 1.0
ren1 SetBackground 0.7 0.5 0.3
renWin Render

exit
