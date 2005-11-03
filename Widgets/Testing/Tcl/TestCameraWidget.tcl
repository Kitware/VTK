package require vtk
package require vtkinteraction
package require vtktesting

# read data
#
vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/Camera.png"

vtkTexture texture
texture SetInput [reader GetOutput]

vtkPlaneSource ps
ps SetOrigin -0.012987 -0.009009 0.0
ps SetPoint2 -0.012987  0.009009 0.0
ps SetPoint1  0.012987 -0.009009 0.0

vtkPolyDataMapper mapper
mapper SetInput [ps GetOutput]

vtkActor actor
actor SetMapper mapper
actor SetTexture texture
[actor GetProperty] SetColor 1 0 0

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground .1 .2 .4

iren Initialize
renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
