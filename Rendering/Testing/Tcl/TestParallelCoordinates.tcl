package require vtk
package require vtkinteraction

# This example converts data to a field and then displays it using 
# parallel coordinates,

# Create a reader and write out the field
vtkUnstructuredGridReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/blow.vtk"
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
    # Set text colors (same as actor for backward compat with test)
    [actor GetTitleTextProperty] SetColor 1 0 0
    [actor GetLabelTextProperty] SetColor 1 0 0

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
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .


