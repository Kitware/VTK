catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# This example converts data to a field and then displays it using 
# parallel coordinates,

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create a reader and write out the field
vtkUnstructuredGridReader reader
    reader SetFileName "$VTK_DATA/blow.vtk"
    reader SetVectorsName displacement9
    reader SetScalarsName thickness9
vtkDataSetToDataObjectFilter ds2do
    ds2do SetInput [reader GetOutput]
vtkParallelCoordinatesActor actor
    actor SetInput [ds2do GetOutput]
    actor SetTitle "Parallel Coordinates Plot of blow.tcl"
    actor SetIndependentVariablesToColumns
    [actor GetPositionCoordinate] SetValue 0.05 0.05 0.0
    [actor GetPosition2Coordinate] SetValue 0.95 0.85 0.0
    [actor GetProperty] SetColor 1 0 0

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 500 200

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .


