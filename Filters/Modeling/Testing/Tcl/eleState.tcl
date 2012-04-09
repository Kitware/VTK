package require vtk
package require vtkinteraction

# This example demonstrates the use of the linear extrusion filter and
# the USA state outline vtk dataset. It also tests the triangulation filter.

# get the interactor ui

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline - read data
#
vtkPolyDataReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/usa.vtk"

# okay, now create some extrusion filters with actors for each US state
vtkMath math
for {set i 0} {$i < 51} {incr i} {
    vtkGeometryFilter extractCell$i
	extractCell$i SetInputConnection [reader GetOutputPort]
        extractCell$i CellClippingOn
        extractCell$i SetCellMinimum $i
        extractCell$i SetCellMaximum $i
    vtkTriangleFilter tf$i
	tf$i SetInputConnection [extractCell$i GetOutputPort]
    vtkLinearExtrusionFilter extrude$i
	extrude$i SetInputConnection [tf$i GetOutputPort]
	extrude$i SetExtrusionType 1
	extrude$i SetVector 0 0 1
	extrude$i CappingOn
	extrude$i SetScaleFactor [math Random 1 10]
    vtkPolyDataMapper mapper$i
	mapper$i SetInputConnection [extrude$i GetOutputPort]
    vtkActor actor$i
	actor$i SetMapper mapper$i
	[actor$i GetProperty] SetColor [math Random 0 1] \
		[math Random 0 1] [math Random 0 1] 

    ren1 AddActor actor$i
}

ren1 SetBackground 1 1 1
renWin SetSize 500 250

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 10.2299 511.497
$cam1 SetPosition -119.669 -25.5502 79.0198
$cam1 SetFocalPoint -115.96 41.6709 1.99546
$cam1 SetViewUp -0.0013035 0.753456 0.657497

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render


# prevent the tk window from showing up then start the event loop
wm withdraw .


