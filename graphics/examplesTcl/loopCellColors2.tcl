catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# include get the vtk interactor ui
source $VTK_TCL/vtkInt.tcl

# create pipeline
#
# create sphere to color
vtkSphereSource sphere1
    sphere1 SetThetaResolution 4
    sphere1 SetPhiResolution 5
sphere1 SetStartTheta 110

vtkSphereSource sphere2
    sphere2 SetThetaResolution 5
    sphere2 SetPhiResolution 5
sphere2 SetStartTheta 0
sphere2 SetEndTheta 110


# Compute random scalars (colors) for each cell
vtkProgrammableAttributeDataFilter randomColors
    randomColors SetInput [sphere1 GetOutput]
    randomColors SetExecuteMethod colorCells

proc colorCells {} {
    vtkMath randomColorGenerator
    set input [randomColors GetInput]
    set output [randomColors GetOutput]
    set numCells [$input GetNumberOfCells]
    vtkScalars colors
	colors SetNumberOfScalars $numCells

    for {set i 0} {$i < $numCells} {incr i} {
        colors SetScalar $i [randomColorGenerator Random .1 .9]
    }

    [$output GetCellData] CopyScalarsOff
    [$output GetCellData] PassData [$input GetCellData]
    [$output GetCellData] SetScalars colors

    colors Delete; #reference counting - it's ok
    randomColorGenerator Delete
}

vtkLoopSubdivisionFilter loopApprox
  loopApprox SetInput [randomColors GetPolyDataOutput]
  loopApprox SetNumberOfSubdivisions 4

# mapper and actor
vtkPolyDataMapper mapper
    mapper SetInput [loopApprox GetOutput]
    eval mapper SetScalarRange [[randomColors GetPolyDataOutput] GetScalarRange]
vtkLoopSubdivisionFilter loop2Approx
  loop2Approx SetInput [sphere2 GetOutput]
  loop2Approx SetNumberOfSubdivisions 4

# mapper and actor
vtkPolyDataMapper mapper2
    mapper2 SetInput [loop2Approx GetOutput]

vtkActor sphere1Actor
    sphere1Actor SetMapper mapper
[sphere1Actor GetProperty] SetDiffuse .7
[sphere1Actor GetProperty] SetSpecular .3
[sphere1Actor GetProperty] SetSpecularPower 20

vtkActor sphere2Actor
    sphere2Actor SetMapper mapper2
[sphere2Actor GetProperty] SetDiffuse .7
[sphere2Actor GetProperty] SetSpecular .3
[sphere2Actor GetProperty] SetSpecularPower 20

vtkExtractEdges edges
  edges SetInput [sphere1 GetOutput]

vtkTubeFilter tubes
  tubes SetInput [edges GetOutput]
  tubes SetRadius .005
  tubes SetNumberOfSides 8

vtkPolyDataMapper tubeMapper
  tubeMapper SetInput [tubes GetOutput]

vtkActor tubeActor
  tubeActor SetMapper tubeMapper

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
ren1 SetBackground .1 .2 .4

vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor sphere1Actor
ren1 AddActor sphere2Actor
ren1 AddActor tubeActor

renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 1.5
renWin Render
renWin SetFileName "loopCellColors2.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

