catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create bottle profile
#
vtkFloatPoints points
    points InsertNextPoint 0 0 0
    points InsertNextPoint 1 0 0
    points InsertNextPoint 1 1 0
    points InsertNextPoint 0 1 0
    points InsertNextPoint 0 1 1
    points InsertNextPoint 1 1 1
    points InsertNextPoint 1 0 1
    points InsertNextPoint 0 0 1
    points InsertNextPoint 0 0 2
    points InsertNextPoint 0 1 2
    points InsertNextPoint 0 1 3
    points InsertNextPoint 0 0 3
    points InsertNextPoint 1 0 3
    points InsertNextPoint 1 1 3
    points InsertNextPoint 1 1 2
    points InsertNextPoint 1 0 2
    points InsertNextPoint 2 0 2
    points InsertNextPoint 2 1 2
    points InsertNextPoint 2 1 3
    points InsertNextPoint 2 0 3
    points InsertNextPoint 3 0 3
    points InsertNextPoint 3 1 3
    points InsertNextPoint 3 1 2
    points InsertNextPoint 3 0 2
    points InsertNextPoint 3 0 1
    points InsertNextPoint 3 0 0
    points InsertNextPoint 2 0 0
    points InsertNextPoint 2 0 1
    points InsertNextPoint 2 1 1
    points InsertNextPoint 2 1 0
    points InsertNextPoint 3 1 0
    points InsertNextPoint 3 1 1
    points InsertNextPoint 3 2 1
    points InsertNextPoint 3 2 0
    points InsertNextPoint 2 2 0
    points InsertNextPoint 2 2 1
    points InsertNextPoint 2 3 1
    points InsertNextPoint 2 3 0
    points InsertNextPoint 3 3 0
    points InsertNextPoint 3 3 1
    points InsertNextPoint 3 3 2
    points InsertNextPoint 3 2 2
    points InsertNextPoint 3 2 3
    points InsertNextPoint 3 3 3
    points InsertNextPoint 2 3 3
    points InsertNextPoint 2 2 3
    points InsertNextPoint 2 2 2
    points InsertNextPoint 2 3 2
    points InsertNextPoint 1 3 2
    points InsertNextPoint 1 2 2
    points InsertNextPoint 1 2 3
    points InsertNextPoint 1 3 3
    points InsertNextPoint 0 3 3
    points InsertNextPoint 0 2 3
    points InsertNextPoint 0 2 2
    points InsertNextPoint 0 3 2
    points InsertNextPoint 0 3 1
    points InsertNextPoint 1 3 1
    points InsertNextPoint 1 2 1
    points InsertNextPoint 0 2 1
    points InsertNextPoint 0 2 0
    points InsertNextPoint 1 2 0
    points InsertNextPoint 1 3 0
    points InsertNextPoint 0 3 0

vtkCellArray lines
    lines InsertNextCell 64;#number of points
    for {set i 0} {$i < 64} {incr i} {
        lines InsertCellPoint $i
    }

vtkPolyData curve
    curve SetPoints points
    curve SetLines lines

vtkTubeFilter tube
    tube SetInput curve
    tube SetNumberOfSides 6
    tube SetRadius 0.05

vtkPolyDataMapper map
    map SetInput [tube GetOutput]

vtkActor peanoCurve
    peanoCurve SetMapper map
    [peanoCurve GetProperty] SetColor 0.3800 0.7000 0.1600
    [peanoCurve GetProperty] BackfaceCullingOn

# Add the actors to the renderer, set the background and size
#
ren1 AddActor peanoCurve
ren1 SetBackground 1 1 1

renWin SetSize 500 500
renWin Render
[ren1 GetActiveCamera] Zoom 1.4
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render
#renWin SetFileName Peano.tcl.ppm
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
