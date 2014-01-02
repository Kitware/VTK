package require vtk
package require vtkinteraction

# Demonstrate how to extract polygonal cells with an implicit function
# get the interactor ui

# create a sphere source and actor
#
vtkSphereSource sphere
  sphere SetThetaResolution 8
  sphere SetPhiResolution 16
  sphere SetRadius 1.5

# Extraction stuff - first just pass points
vtkTransform t
    t RotateX 90
vtkCylinder cylfunc
    cylfunc SetRadius 0.5
    cylfunc SetTransform t
vtkExtractPolyDataGeometry extract
    extract SetInputConnection [sphere GetOutputPort]
    extract SetImplicitFunction cylfunc
    extract ExtractBoundaryCellsOn
    extract PassPointsOn
vtkPolyDataMapper  sphereMapper
    sphereMapper SetInputConnection [extract GetOutputPort]
    sphereMapper GlobalImmediateModeRenderingOn
vtkActor sphereActor
    sphereActor SetMapper sphereMapper

# Extraction stuff - now cull points
vtkExtractPolyDataGeometry extract2
    extract2 SetInputConnection [sphere GetOutputPort]
    extract2 SetImplicitFunction cylfunc
    extract2 ExtractBoundaryCellsOn
    extract2 PassPointsOff
vtkPolyDataMapper  sphereMapper2
    sphereMapper2 SetInputConnection [extract2 GetOutputPort]
vtkActor sphereActor2
    sphereActor2 SetMapper sphereMapper2
    sphereActor2 AddPosition 2.5 0 0

# Put some glyphs on the points
vtkSphereSource glyphSphere
   glyphSphere SetRadius 0.05
vtkGlyph3D glyph
    glyph SetInputConnection [extract GetOutputPort]
    glyph SetSourceConnection [glyphSphere GetOutputPort]
    glyph SetScaleModeToDataScalingOff
vtkPolyDataMapper  glyphMapper
    glyphMapper SetInputConnection [glyph GetOutputPort]
vtkActor glyphActor
    glyphActor SetMapper glyphMapper

vtkGlyph3D glyph2
    glyph2 SetInputConnection [extract2 GetOutputPort]
    glyph2 SetSourceConnection [glyphSphere GetOutputPort]
    glyph2 SetScaleModeToDataScalingOff
vtkPolyDataMapper  glyphMapper2
    glyphMapper2 SetInputConnection [glyph2 GetOutputPort]
vtkActor glyphActor2
    glyphActor2 SetMapper glyphMapper2
    glyphActor2 AddPosition 2.5 0 0

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetWindowName "vtk - extractPolyData"
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor
ren1 AddActor glyphActor
ren1 AddActor sphereActor2
ren1 AddActor glyphActor2

ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth 30

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300
renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


