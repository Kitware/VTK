# This example shows how to sample a mathematical function over a
# volume. A slice from the volume is then extracted and then contoured
# to produce 2D contour lines.
#
package require vtk
package require vtkinteraction

# Quadric definition. This is a type of implicit function. Here the 
# coefficients to the equations are set.
vtkQuadric quadric
  quadric SetCoefficients .5 1 .2 0 .1 0 0 .2 0 0

# The vtkSampleFunction uses the quadric function and evaluates function
# value over a regular lattice (i.e., a volume).
vtkSampleFunction sample
  sample SetSampleDimensions 30 30 30
  sample SetImplicitFunction quadric
  sample ComputeNormalsOff

# Here a single slice (i.e., image) is extracted from the volume. (Note: in
# actuality the VOI request causes the sample function to operate on just the
# slice.)
vtkExtractVOI extract
  extract SetInputConnection [sample GetOutputPort]
  extract SetVOI 0 29 0 29 15 15
  extract SetSampleRate 1 2 3

# The image is contoured to produce contour lines. Thirteen contour values
# ranging from (0,1.2) inclusive are produced.
vtkContourFilter contours
  contours SetInputConnection [extract GetOutputPort]
  contours GenerateValues 13 0.0 1.2

# The contour lines are mapped to the graphics library.
vtkPolyDataMapper contMapper
  contMapper SetInputConnection [contours GetOutputPort]
  contMapper SetScalarRange 0.0 1.2

vtkActor contActor
  contActor SetMapper contMapper

# Create outline an outline of the sampled data.
vtkOutlineFilter outline
  outline SetInputConnection [sample GetOutputPort]

vtkPolyDataMapper outlineMapper
  outlineMapper SetInputConnection [outline GetOutputPort]

vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  eval [outlineActor GetProperty] SetColor 0 0 0

# Create the renderer, render window, and interactor.
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Set the background color to white. Associate the actors with the
# renderer.
ren1 SetBackground 1 1 1
ren1 AddActor contActor
ren1 AddActor outlineActor

# Zoom in a little bit. Associate the Tk interactor popup with a user
# keypress-u (the UserEvent).
ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 1.5
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize;

# Don't show the root Tk window "."
wm withdraw .
