catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# demonstrate the use of 2D text

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

vtkSphereSource sphere

vtkPolyDataMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn
vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper
    [sphereActor GetProperty] SetOpacity 0.75

vtkTextMapper foregroundTextMapper
    foregroundTextMapper SetInput "this text is in the foreground"
    foregroundTextMapper SetFontSize 12
    foregroundTextMapper SetFontFamilyToArial
    foregroundTextMapper SetJustificationToCentered
    foregroundTextMapper BoldOn
    foregroundTextMapper ItalicOn
    foregroundTextMapper ShadowOn
vtkActor2D foregroundTextActor
    foregroundTextActor SetMapper foregroundTextMapper    
    [foregroundTextActor GetPositionCoordinate] \
          SetCoordinateSystemToNormalizedDisplay
    [foregroundTextActor GetPositionCoordinate] SetValue 0.5 0.25
    [foregroundTextActor GetProperty] SetColor 1 0 0
    [foregroundTextActor GetProperty] SetDisplayLocationToForeground

vtkTextMapper defaultTextMapper
    defaultTextMapper SetInput "this text is in the default position"
    defaultTextMapper SetFontSize 12
    defaultTextMapper SetFontFamilyToArial
    defaultTextMapper SetJustificationToCentered
    defaultTextMapper BoldOn
    defaultTextMapper ItalicOn
    defaultTextMapper ShadowOn
vtkActor2D defaultTextActor
    defaultTextActor SetMapper defaultTextMapper    
    [defaultTextActor GetPositionCoordinate] \
          SetCoordinateSystemToNormalizedDisplay
    [defaultTextActor GetPositionCoordinate] SetValue 0.5 0.50
    [defaultTextActor GetProperty] SetColor 0 1 0

vtkTextMapper backgroundTextMapper
    backgroundTextMapper SetInput "this text is in the background"
    backgroundTextMapper SetFontSize 12
    backgroundTextMapper SetFontFamilyToArial
    backgroundTextMapper SetJustificationToCentered
    backgroundTextMapper BoldOn
    backgroundTextMapper ItalicOn
    backgroundTextMapper ShadowOn
vtkActor2D backgroundTextActor
    backgroundTextActor SetMapper backgroundTextMapper    
    [backgroundTextActor GetPositionCoordinate] \
          SetCoordinateSystemToNormalizedDisplay
    [backgroundTextActor GetPositionCoordinate] SetValue 0.5 0.75
    [backgroundTextActor GetProperty] SetColor 0 0 1
    [backgroundTextActor GetProperty] SetDisplayLocationToBackground

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddProp foregroundTextActor
ren1 AddProp defaultTextActor
ren1 AddProp backgroundTextActor
ren1 AddProp sphereActor

ren1 SetBackground 1 1 1
[ren1 GetActiveCamera] Zoom 1.3
ren1 ResetCameraClippingRange
renWin Render
 
renWin SetFileName "TestFGBGTextMapper.tcl.ppm"
#renWin SaveImageAsPPM

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
  
# prevent the tk window from showing up then start the event loop
wm withdraw .



