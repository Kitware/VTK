catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# user interface command widget
source $VTK_TCL/vtkInt.tcl

# create a rendering window and renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1

# set camera for a parallel, oblique projection
vtkCamera camera
    camera ParallelProjectionOn
    camera SetParallelScale 1
    camera SetObliqueAngles 30 63.43
    camera SetPosition 0 0 5
ren1 SetActiveCamera camera

# set light along view plane normal
vtkLight light
    light SetPosition 2.17 1.25 5
ren1 AddLight light

# set up first set of polydata
vtkCubeSource cube

vtkDataSetMapper cubeMapper
  cubeMapper SetInput [cube GetOutput]
vtkActor cubeActor
  cubeActor SetMapper cubeMapper
  [cubeActor GetProperty] BackfaceCullingOn

# assign our actor to the renderer
ren1 AddActor cubeActor

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .
