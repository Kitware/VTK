package require vtk
package require vtkinteraction

# create pipeline
#
# create sphere to color
vtkSphereSource sphere
    sphere SetThetaResolution 20
    sphere SetPhiResolution 40

# Compute random scalars (colors) for each cell
vtkProgrammableAttributeDataFilter randomColors
    randomColors SetInput [sphere GetOutput]
    randomColors SetExecuteMethod colorCells

proc colorCells {} {
    vtkMath randomColorGenerator
    set input [randomColors GetInput]
    set output [randomColors GetOutput]
    set numCells [$input GetNumberOfCells]
    vtkFloatArray colors
	colors SetNumberOfTuples $numCells

    for {set i 0} {$i < $numCells} {incr i} {
        colors SetValue $i [randomColorGenerator Random 0 1]
    }

    [$output GetCellData] CopyScalarsOff
    [$output GetCellData] PassData [$input GetCellData]
    [$output GetCellData] SetScalars colors

    colors Delete; #reference counting - it's ok
    randomColorGenerator Delete
}
# mapper and actor
vtkPolyDataMapper mapper
    mapper SetInput [randomColors GetPolyDataOutput]
    eval mapper SetScalarRange [[randomColors GetPolyDataOutput] GetScalarRange]
vtkActor sphereActor
    sphereActor SetMapper mapper

# Create a scalar bar
vtkScalarBarActor scalarBar
    scalarBar SetLookupTable [mapper GetLookupTable]
    scalarBar SetTitle "Temperature"
    [scalarBar GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport
    [scalarBar GetPositionCoordinate] SetValue 0.1 0.05
    scalarBar SetOrientationToVertical
    scalarBar SetWidth 0.8
    scalarBar SetHeight 0.9
    scalarBar SetLabelFormat "%-#6.3f"

# Test the Get/Set Position 
eval scalarBar SetPosition  [scalarBar GetPosition]

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderer ren2
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin AddRenderer ren2
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor sphereActor
ren2 AddActor2D scalarBar
renWin SetSize 160 160
ren1 SetViewport 0 0 0.75 1.0
ren2 SetViewport 0.75 0 1.0 1.0
ren2 SetBackground 0.3 0.3 0.3

# render the image
#
renWin Render
scalarBar SetNumberOfLabels 8
renWin Render


vtkWindowToImageFilter w2i
w2i SetInput renWin
w2i SetMagnification 2
w2i Update
w2i SetInput {}


vtkImageMapper ia
ia SetInput [w2i GetOutput]
[w2i GetOutput] SetSource {}
scalarBar ReleaseGraphicsResources renWin
sphereActor ReleaseGraphicsResources renWin
ia SetColorWindow 255
ia SetColorLevel 127.5

vtkActor2D ia2
ia2 SetMapper ia

renWin SetSize 320 320
ren2 RemoveProp scalarBar
ren1 RemoveProp sphereActor
ren1 AddActor ia2
renWin RemoveRenderer ren2 
ren1 SetViewport 0 0 1 1

renWin Render
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

