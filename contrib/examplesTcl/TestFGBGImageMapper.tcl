catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

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
  reader SetDataExtent 0 255 0 255 1 93
  reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader SetDataMask 0x7fff
  reader Update

vtkImageMapper backgroundMapper
backgroundMapper SetInput [reader GetOutput]
backgroundMapper SetColorWindow 2000
backgroundMapper SetColorLevel 1000
backgroundMapper SetZSlice 30

vtkImageMapper foregroundMapper
foregroundMapper SetInput [reader GetOutput]
foregroundMapper SetColorWindow 2000
foregroundMapper SetColorLevel 1000
foregroundMapper SetZSlice 60

vtkImageMapper defaultMapper
defaultMapper SetInput [reader GetOutput]
defaultMapper SetColorWindow 2000
defaultMapper SetColorLevel 1000
defaultMapper SetZSlice 90

vtkActor2D backgroundActor
backgroundActor SetMapper backgroundMapper
[backgroundActor GetProperty] SetDisplayLocationToBackground
[backgroundActor GetPositionCoordinate] \
      SetCoordinateSystemToNormalizedDisplay
[backgroundActor GetPositionCoordinate] SetValue 0.0 0.0

vtkActor2D foregroundActor
foregroundActor SetMapper foregroundMapper
[foregroundActor GetProperty] SetDisplayLocationToForeground
[foregroundActor GetPositionCoordinate] \
      SetCoordinateSystemToNormalizedDisplay
[foregroundActor GetPositionCoordinate] SetValue 0.5 0.0

vtkActor2D defaultActor
defaultActor SetMapper defaultMapper
[defaultActor GetPositionCoordinate] \
      SetCoordinateSystemToNormalizedDisplay
[defaultActor GetPositionCoordinate] SetValue 0.0 0.5

vtkSphereSource sphereSource

vtkPolyDataMapper sphereMapper
sphereMapper SetInput [sphereSource GetOutput]

vtkActor sphereActor
sphereActor SetMapper sphereMapper

ren1 AddProp backgroundActor
ren1 AddProp foregroundActor
ren1 AddProp defaultActor
ren1 AddProp sphereActor
ren1 SetBackground 1 1 1

renWin SetSize 512 512
renWin Render

#renWin SetFileName "TestFGBGImageMapper.tcl.ppm"
#renWin SaveImageAsPPM

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



