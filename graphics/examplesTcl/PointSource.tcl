catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# This example demonstrates how to generate a point cloud using VTK

# create a rendering window
vtkRenderWindow renWin
renWin SetSize 400 250
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create renderers
vtkRenderer ren1
ren1 SetViewport 0 0 0.5 1
renWin AddRenderer ren1

vtkRenderer ren2
ren2 SetViewport 0.5 0 1 1
renWin AddRenderer ren2

# create pipelines
vtkPointSource points1
points1 SetNumberOfPoints 2000
points1 SetRadius 1.5
points1 SetCenter 0 0 0

vtkDataSetMapper mapper1
mapper1 SetInput [points1 GetOutput]

vtkActor actor1
actor1 SetMapper mapper1

ren1 AddActor actor1


vtkPointSource points2
catch {points2 SetDistributionToShell}
points2 SetNumberOfPoints 2000

vtkDataSetMapper mapper2
mapper2 SetInput [points2 GetOutput]

vtkActor actor2
actor2 SetMapper mapper2

ren2 AddActor actor2


wm withdraw .
iren Initialize