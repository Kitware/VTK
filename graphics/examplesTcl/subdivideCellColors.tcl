
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# include get the vtk interactor ui
source $VTK_TCL/vtkInt.tcl

# create pipeline
#
# create sphere to color
vtkSphereSource sphere
    sphere SetThetaResolution 0
    sphere SetPhiResolution 0

# Compute random scalars (colors) for each cell
vtkProgrammableAttributeDataFilter randomColors
    randomColors SetInput [sphere GetOutput]
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
[[randomColors GetOutput] GetPointData] CopyNormalsOff

vtkButterflySubdivisionFilter linear
  linear SetInput [randomColors GetPolyDataOutput]
  linear SetNumberOfSubdivisions 4

# mapper and actor
vtkPolyDataMapper mapper
    mapper SetInput [linear GetOutput]
    eval mapper SetScalarRange [[randomColors GetPolyDataOutput] GetScalarRange]
vtkActor sphereActor
    sphereActor SetMapper mapper
[sphereActor GetProperty] SetDiffuse .7
[sphereActor GetProperty] SetSpecular .4
[sphereActor GetProperty] SetSpecularPower 60

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor sphereActor
renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 1.5
renWin Render
renWin SetFileName "subdivideCellColors.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

