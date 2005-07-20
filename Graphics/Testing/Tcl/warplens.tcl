package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# load in the texture map
#
vtkPNGReader pngReader
pngReader SetFileName "$VTK_DATA_ROOT/Data/camscene.png"
pngReader Update

set xWidth [lindex [[pngReader GetOutput] GetDimensions] 0]
set yHeight [lindex [[pngReader GetOutput] GetDimensions] 1]

vtkGeometryFilter gf
gf SetInputConnection [pngReader GetOutputPort]

vtkWarpLens wl
wl SetInputConnection [gf GetOutputPort]

wl SetPrincipalPoint 2.4507 1.7733
wl SetFormatWidth 4.792
wl SetFormatHeight 3.6
wl SetImageWidth $xWidth
wl SetImageHeight $yHeight
wl SetK1 0.01307
wl SetK2 0.0003102
wl SetP1 1.953e-005
wl SetP2 -9.655e-005

vtkTriangleFilter tf
tf SetInput [wl GetPolyDataOutput]

vtkStripper strip
strip SetInputConnection [tf GetOutputPort]
strip SetMaximumLength 250

vtkPolyDataMapper dsm
dsm SetInputConnection [strip GetOutputPort]

vtkActor planeActor
planeActor SetMapper dsm

# Add the actors to the renderer, set the background and size
ren1 AddActor planeActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render
[ren1 GetActiveCamera] Zoom 1.4
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .





