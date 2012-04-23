package require vtk
package require vtkinteraction

# Create the RenderWindow and Renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetAlphaBitPlanes 1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a default polygonal sphere
vtkSphereSource sphere
vtkPolyDataMapper sphmapper
   sphmapper SetInputConnection [sphere GetOutputPort]
vtkActor sphactor
   sphactor SetMapper sphmapper

# Add the actors to the renderer, set the background to initial
# color (which is also transparent), set size.
ren1 AddActor sphactor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 256 256

# render first image
renWin Render

if {0 == [renWin GetAlphaBitPlanes]} {
  puts "Failed to find a visual with alpha bit planes."
  exit 0
} {
  puts [concat "GetAlphaBitPlanes: " [renWin GetAlphaBitPlanes]]
}

# create window to image filter, grabbing RGB and alpha
vtkWindowToImageFilter w2i
   w2i SetInput renWin
   w2i SetInputBufferTypeToRGBA

# grab window
w2i Update

# copy the output
set outputData [[w2i GetOutput] NewInstance]
$outputData DeepCopy [w2i GetOutput]

# set up mappers and actors to display the image
vtkImageMapper im
   im SetColorWindow 255
   im SetColorLevel 127.5
   im SetInputData $outputData
vtkActor2D ia2
   ia2 SetMapper im

# now, change the image (background is now green)
sphactor SetScale 2 2 2
ren1 SetBackground 0 1 0

# add the image of the sphere (keeping the original sphere too)
ren1 AddActor ia2
ren1 SetViewport 0 0 1 1

# render result (the polygonal sphere appears behind a smaller image
# of itself).  Background of original image is transparent, so you
# can see through it back to the larger sphere and new background.

renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

$outputData UnRegister {}
