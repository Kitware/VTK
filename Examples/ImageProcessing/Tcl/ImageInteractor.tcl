# This example shows how to use the InteractorStyleImage.
# The InteractorStyleImage is a special interactor designed
# to be used with vtkImageActor in a rendering window context.

package require vtk
package require vtkinteraction

# Create the image
#
set RANGE            150
set MAX_ITERATIONS_1 $RANGE
set MAX_ITERATIONS_2 $RANGE
set XRAD             600
set YRAD             600
set sample [expr 1.3 / $XRAD]

# Create a Mandelbrot set of appropriate resolution
vtkImageMandelbrotSource mandelbrot1
  mandelbrot1 SetMaximumNumberOfIterations [expr int($MAX_ITERATIONS_1)]
  mandelbrot1 SetWholeExtent [expr -$XRAD] [expr $XRAD-1] \
                            [expr -$YRAD] [expr $YRAD-1] 0 0
  mandelbrot1 SetSampleCX $sample $sample $sample $sample 
  mandelbrot1 SetOriginCX -0.72 0.22  0.0 0.0
  mandelbrot1 SetProjectionAxes 0 1 2

vtkLookupTable table1
  table1 SetTableRange 0 $RANGE
  table1 SetNumberOfColors $RANGE
  table1 Build
  table1 SetTableValue [expr $RANGE - 1]  0.0 0.0 0.0 0.0

vtkImageMapToRGBA map1
  map1 SetInput [mandelbrot1 GetOutput]
  map1 SetLookupTable table1

vtkImageActor ia
ia SetInput [map1 GetOutput]

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Create an image interactor
vtkInteractorStyleImage interactor
iren SetInteractorStyle interactor

# Add the actors to the renderer, set the background and size
ren1 AddActor ia
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 400 400

# render the image
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

set cam1 [ren1 GetActiveCamera]

ren1 ResetCameraClippingRange
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .





