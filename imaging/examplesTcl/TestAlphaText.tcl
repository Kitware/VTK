catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# demonstrate the use of 2D text

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

vtkSphereSource sphere
sphere SetThetaResolution 20
sphere SetPhiResolution 20

vtkPolyDataMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn
vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper
    [sphereActor GetProperty] SetColor 0.5 0.3 0.1

vtkTextMapper textMapper1
    textMapper1 SetInput "This is a sphere"
    textMapper1 SetFontSize 32
    textMapper1 SetFontFamilyToArial
    textMapper1 SetJustificationToCentered
    textMapper1 BoldOn
    textMapper1 ItalicOn
    textMapper1 ShadowOn
vtkActor2D textActor1
    textActor1 SetMapper textMapper1    
    [textActor1 GetPositionCoordinate] \
          SetCoordinateSystemToNormalizedDisplay
    [textActor1 GetPositionCoordinate] SetValue 0.5 0.25
    [textActor1 GetProperty] SetColor 0 0 1
    [textActor1 GetProperty] SetOpacity 0.25

vtkTextMapper textMapper2
    textMapper2 SetInput "This is a sphere"
    textMapper2 SetFontSize 32
    textMapper2 SetFontFamilyToArial
    textMapper2 SetJustificationToCentered
    textMapper2 BoldOn
    textMapper2 ItalicOn
    textMapper2 ShadowOn
vtkActor2D textActor2
    textActor2 SetMapper textMapper2    
    [textActor2 GetPositionCoordinate] \
          SetCoordinateSystemToNormalizedDisplay
    [textActor2 GetPositionCoordinate] SetValue 0.5 0.5
    [textActor2 GetProperty] SetColor 0 0 1
    [textActor2 GetProperty] SetOpacity 0.5

vtkTextMapper textMapper3
    textMapper3 SetInput "This is a sphere"
    textMapper3 SetFontSize 32
    textMapper3 SetFontFamilyToArial
    textMapper3 SetJustificationToCentered
    textMapper3 BoldOn
    textMapper3 ItalicOn
    textMapper3 ShadowOn
vtkActor2D textActor3
    textActor3 SetMapper textMapper3    
    [textActor3 GetPositionCoordinate] \
          SetCoordinateSystemToNormalizedDisplay
    [textActor3 GetPositionCoordinate] SetValue 0.5 0.75
    [textActor3 GetProperty] SetColor 0 0 1
    [textActor3 GetProperty] SetOpacity 0.75


# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor2D textActor1
ren1 AddActor2D textActor2
ren1 AddActor2D textActor3
ren1 AddActor sphereActor

ren1 SetBackground 1 1 1
renWin SetSize 300 300
[ren1 GetActiveCamera] Zoom 1.5
renWin Render

#renWin SetFileName "TestText.tcl.ppm"
#renWin SaveImageAsPPM

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

renWin Render

## Write the image
##vtkWindowToImageFilter w2i
##w2i SetInput renWin

##vtkTIFFWriter tiff
##tiff SetInput [w2i GetOutput]
##tiff SetFileName TestAlphaText.tcl.tif
##tiff Write

