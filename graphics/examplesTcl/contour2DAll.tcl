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
vtkSLCReader slc
  slc SetFileName "$VTK_DATA/vm_foot.slc"

set types  "Char UnsignedChar Short UnsignedShort Int UnsignedInt Long UnsignedLong Float Double"
set i 1
foreach vtkType $types {
  vtkImageClip clip$vtkType
    clip$vtkType SetInput [slc GetOutput]
    clip$vtkType SetOutputWholeExtent -1000 1000 -1000 1000 $i $i
  incr i 22
  vtkImageCast castTo$vtkType
    castTo$vtkType SetOutputScalarTypeTo$vtkType
    castTo$vtkType SetInput [clip$vtkType GetOutput]
    castTo$vtkType ClampOverflowOn

  vtkContourFilter iso$vtkType
    iso$vtkType SetInput [castTo$vtkType GetOutput]
    iso$vtkType GenerateValues 1 30 30

  vtkPolyDataMapper iso${vtkType}Mapper
    iso${vtkType}Mapper SetInput [iso$vtkType GetOutput]
    iso${vtkType}Mapper SetColorModeToMapScalars

  vtkActor iso${vtkType}Actor
    iso${vtkType}Actor SetMapper iso${vtkType}Mapper
    ren1 AddActor iso${vtkType}Actor
}

vtkOutlineFilter outline
  outline SetInput [slc GetOutput]
vtkPolyDataMapper outlineMapper
  outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  outlineActor VisibilityOff

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
[ren1 GetActiveCamera] SetFocalPoint 80 130 106
[ren1 GetActiveCamera] SetPosition -170 -305 -131
[ren1 GetActiveCamera] SetViewUp 0 0 -1
[ren1 GetActiveCamera] SetViewAngle 30
ren1 ResetCameraClippingRange

ren1 SetBackground 0.9 .9 .9
renWin SetSize 450 450
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin SetFileName "contour2DAll.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


