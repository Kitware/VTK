# This example demonstrates the use of the contour filter, and the use of
# the vtkSampleFunction to generate a volume of data samples from an
# implicit function.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands from Tcl. The vtkinteraction package defines
# a simple Tcl/Tk interactor widget.
#
package require vtk
package require vtkinteraction

# VTK supports implicit functions of the form f(x,y,z)=constant. These
# functions can represent things spheres, cones, etc. Here we use a
# general form for a quadric to create an elliptical data field.
vtkQuadric quadric
  quadric SetCoefficients .5 1 .2 0 .1 0 0 .2 0 0

# vtkSampleFunction samples an implicit function over the x-y-z range
# specified (here it defaults to -1,1 in the x,y,z directions).
vtkSampleFunction sample
  sample SetSampleDimensions 30 30 30
  sample SetImplicitFunction quadric

# Create five surfaces F(x,y,z) = constant between range specified. The
# GenerateValues() method creates n isocontour values between the range
# specified.
vtkContourFilter contours
  contours SetInputConnection [sample GetOutputPort]
  contours GenerateValues 5 0.0 1.2

vtkPolyDataMapper contMapper
  contMapper SetInputConnection [contours GetOutputPort]
  contMapper SetScalarRange 0.0 1.2

vtkActor contActor
  contActor SetMapper contMapper

# We'll put a simple outline around the data.
vtkOutlineFilter outline
  outline SetInputConnection [sample GetOutputPort]

vtkPolyDataMapper outlineMapper
  outlineMapper SetInputConnection [outline GetOutputPort]

vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  eval [outlineActor GetProperty] SetColor 0 0 0

# The usual rendering stuff.
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 SetBackground 1 1 1
ren1 AddActor contActor
ren1 AddActor outlineActor

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .
iren Start