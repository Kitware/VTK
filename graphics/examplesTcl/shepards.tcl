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

# create some points
#
vtkMath math
vtkFloatPoints points
for {set i 0} {$i<50} {incr i 1} {
    eval points InsertPoint $i [math Random 0 1] [math Random 0 1] [math Random 0 1]
}

vtkFloatScalars scalars
for {set i 0} {$i<50} {incr i 1} {
    eval scalars InsertScalar $i [math Random 0 1]
}

vtkPolyData profile
    profile SetPoints points
    [profile GetPointData] SetScalars scalars

# triangulate them
#
vtkShepardMethod shepard
    shepard SetInput profile
    shepard SetModelBounds 0 1 0 1 .1 .5
#    shepard SetMaximumDistance .1
    shepard SetNullValue 1
    shepard SetSampleDimensions 20 20 20
    shepard Update
    
vtkDataSetMapper map
    map SetInput [shepard GetOutput]

vtkActor block
    block SetMapper map
    [block GetProperty] SetColor 1 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor block
ren1 SetBackground 1 1 1
renWin SetSize 500 500

set cam1 [ren1 GetActiveCamera]
$cam1 Azimuth 160
$cam1 Elevation 30
$cam1 Zoom 1.5

renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render

#renWin SetFileName shepards.tcl.ppm
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


