package require vtktcl_interactor

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkSphereSource sphere
vtkPolyDataMapper sphereMapper
    sphereMapper SetInput [sphere GetOutput]
vtkActor sphereActor
    sphereActor SetMapper sphereMapper

vtkConeSource coneGlyph
    coneGlyph SetResolution 6

vtkSphereSource sphereGlyph
    sphereGlyph SetThetaResolution 12
    sphereGlyph SetPhiResolution 6

vtkCaptionActor2D caption
    caption SetCaption "This is the\nsouth pole"
    [caption GetProperty] SetColor 1 0 0
    caption SetAttachmentPoint 0 0 -0.5
    caption SetJustificationToCentered
    [caption GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport
    [caption GetPositionCoordinate] SetReferenceCoordinate ""
    [caption GetPositionCoordinate] SetValue 0.05 0.05
    caption SetWidth 0.15
    caption SetHeight 0.05
    caption ThreeDimensionalLeaderOn
    caption SetLeaderGlyph [coneGlyph GetOutput]

vtkCaptionActor2D caption2
    caption2 SetCaption "Santa lives here"
    [caption2 GetProperty] SetColor 1 0 0
    caption2 SetAttachmentPoint 0 0 0.5
    caption2 SetHeight 0.05
    caption2 BorderOff
    caption2 SetPosition 25 10
    caption2 ThreeDimensionalLeaderOff
    caption2 SetLeaderGlyph [sphereGlyph GetOutput]

ren1 AddActor sphereActor
ren1 AddActor2D caption2
ren1 AddActor2D caption
ren1 SetBackground 1 1 1
[ren1 GetActiveCamera] SetFocalPoint 0 0 0
[ren1 GetActiveCamera] SetPosition 1 0 0
[ren1 GetActiveCamera] SetViewUp 0 0 1
ren1 ResetCamera

renWin SetSize 250 250

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .


