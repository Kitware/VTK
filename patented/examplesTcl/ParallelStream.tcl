# This example test the ParallelStreaming flag in the 
# vtkAppendPolyData filter.

# parameters
set NUMBER_OF_PIECES 13

catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkImageReader reader
    reader SetDataByteOrderToLittleEndian
    reader SetDataExtent 0 127 0 127 1 93
    reader SetFilePrefix "$VTK_DATA/headsq/half"
    reader SetDataSpacing 1.6 1.6 1.5

vtkAppendPolyData app
    app ParallelStreamingOn

for {set i 0} {$i < $NUMBER_OF_PIECES} {incr i} {
#  vtkContourFilter iso$i
  vtkSynchronizedTemplates3D iso$i
    iso$i SetInput [reader GetOutput]
    iso$i SetValue 0 500
    iso$i ComputeScalarsOff
    iso$i ComputeGradientsOff
    iso$i SetNumberOfThreads 1

  if {$NUMBER_OF_PIECES == 1} {
     set val 0.0
  } else {
     set val [expr 0.0 + $i / ($NUMBER_OF_PIECES - 1.0)]
  }
  vtkElevationFilter elev$i
    elev$i SetInput [iso$i GetOutput]
    elev$i SetScalarRange $val [expr $val+0.001]

  app AddInput [elev$i GetOutput]
}

vtkPolyDataMapper mapper
  mapper SetInput [app GetOutput]
  mapper ImmediateModeRenderingOn

vtkActor actor
  actor SetMapper mapper
  [actor GetProperty] SetSpecularPower 30
  [actor GetProperty] SetDiffuse .7
  [actor GetProperty] SetSpecular .5
    
ren1 AddActor actor

vtkOutlineFilter outline
  outline SetInput [reader GetOutput]
vtkPolyDataMapper outlineMapper
  outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  outlineActor VisibilityOff

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 SetBackground 0.9 .9 .9
[ren1 GetActiveCamera] SetFocalPoint 100 100 65
[ren1 GetActiveCamera] SetPosition 100 450 65
[ren1 GetActiveCamera] SetViewUp 0 0 -1
[ren1 GetActiveCamera] SetViewAngle 30
ren1 ResetCameraClippingRange

renWin SetSize 450 450
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin SetFileName "ParallelStream.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


