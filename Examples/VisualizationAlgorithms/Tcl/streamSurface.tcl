# This example demonstrates the generation of a streamsurface.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands from Tcl. The vtkinteraction package defines
# a simple Tcl/Tk interactor widget.
#
package require vtk
package require vtkinteraction
package require vtktesting

# Read the data and specify which scalars and vectors to read.
#
vtkPLOT3DReader pl3d
  pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
  pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
  pl3d SetScalarFunctionNumber 100 
  pl3d SetVectorFunctionNumber 202
  pl3d Update

# We use a rake to generate a series of streamline starting points
# scattered along a line. Each point will generate a streamline. These
# streamlines are then fed to the vtkRuledSurfaceFilter which stitches
# the lines together to form a surface.
#
vtkLineSource rake
  rake SetPoint1 15 -5 32
  rake SetPoint2 15 5 32
  rake SetResolution 21
vtkPolyDataMapper rakeMapper
  rakeMapper SetInput [rake GetOutput]
vtkActor rakeActor
  rakeActor SetMapper rakeMapper 

vtkRungeKutta4 integ
vtkStreamLine sl
  sl SetInput [pl3d GetOutput]
  sl SetSource [rake GetOutput] 
  sl SetIntegrator integ 
  sl SetMaximumPropagationTime 0.1 
  sl SetIntegrationStepLength 0.1 
  sl SetIntegrationDirectionToBackward
  sl SetStepLength 0.001 

#
# The ruled surface stiches together lines with triangle strips.
# Note the SetOnRatio method. It turns on every other strip that
# the filter generates (only when multiple lines are input).
#
vtkRuledSurfaceFilter scalarSurface
  scalarSurface SetInput [sl GetOutput]
  scalarSurface SetOffset 0 
  scalarSurface SetOnRatio 2 
  scalarSurface PassLinesOn
  scalarSurface SetRuledModeToPointWalk
  scalarSurface SetDistanceFactor 30 
vtkPolyDataMapper mapper
  mapper SetInput [scalarSurface GetOutput]
  eval mapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor actor
  actor SetMapper mapper 

# Put an outline around for context.
#
vtkStructuredGridOutlineFilter outline
  outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
  outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
  outlineActor SetMapper outlineMapper 
  [outlineActor GetProperty] SetColor 0 0 0

# Now create the usual graphics stuff.
vtkRenderer ren
vtkRenderWindow renWin
    renWin AddRenderer ren
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren AddActor rakeActor 
ren AddActor actor 
ren AddActor outlineActor 
ren SetBackground 1 1 1

renWin SetSize 300 300

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# interact with data
wm withdraw .

