catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# This example demonstrates the reading of point data and cell data
# simultaneously, and then the querying of this information.

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# create pipeline
#
vtkPolyDataReader reader
    reader SetFileName "$VTK_DATA/polyEx.vtk"
    reader SetScalarsName $scalar0
    set numScalars [reader GetNumberOfScalarsInFile]
    set numVectors [reader GetNumberOfVectorsInFile]
    set numNormals [reader GetNumberOfNormalsInFile]
    reader Modified
    set numScalars [reader GetNumberOfScalarsInFile]
    set scalar0 [reader GetScalarsNameInFile 1]

vtkPolyDataMapper mapper
    mapper SetInput [reader GetOutput]
    eval mapper SetScalarRange [[reader GetOutput] GetScalarRange]
vtkActor actor
    actor SetMapper mapper

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 500 500

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange .348 17.43
$cam1 SetPosition 2.92 2.62 -0.836
$cam1 SetViewUp -0.436 -0.067 -0.897

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .


