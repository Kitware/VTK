catch {load vtktcl}
# demonstrate the use of 2D text

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

vtkSphereSource sphere

vtkPolyDataMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn
vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper

vtkTextMapper textMapper
    textMapper SetInput "This is a sphere"
    textMapper SetFontSize 18
    textMapper SetFontFamilyToArial
    textMapper BoldOn
    textMapper ItalicOn
    textMapper ShadowOn
vtkActor2D textActor
    textActor SetMapper textMapper    
    textActor SetPosition 90 50
    [textActor GetProperty] SetColor 0 0 1

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor2D textActor
ren1 AddActor sphereActor

ren1 SetBackground 1 1 1
renWin SetSize 250 125
[ren1 GetActiveCamera] Zoom 1.5
renWin Render

#renWin SetFileName "TestText.tcl.ppm"
#renWin SaveImageAsPPM

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



