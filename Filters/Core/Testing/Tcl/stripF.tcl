package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a cyberware source
#
vtkPolyDataReader cyber
    cyber SetFileName "$VTK_DATA_ROOT/Data/fran_cut.vtk"
vtkPolyDataNormals normals;#enable this for cool effect
    normals SetInputConnection [cyber GetOutputPort]
    normals FlipNormalsOn
vtkStripper stripper
    stripper SetInputConnection [cyber GetOutputPort]
vtkMaskPolyData mask
    mask SetInputConnection [stripper GetOutputPort]
    mask SetOnRatio 2
vtkPolyDataMapper cyberMapper
    cyberMapper SetInputConnection [mask GetOutputPort]
vtkActor cyberActor
    cyberActor SetMapper cyberMapper
    [cyberActor GetProperty] SetColor 1.0 0.49 0.25

# Add the actors to the renderer, set the background and size
#
ren1 AddActor cyberActor
ren1 SetBackground 1 1 1
renWin SetSize 300 300
#ren1 SetBackground 0.1 0.2 0.4
ren1 SetBackground 1 1 1

# render the image
#
vtkCamera cam1
  cam1 SetFocalPoint 0.0520703 -0.128547 -0.0581083
  cam1 SetPosition 0.419653 -0.120916 -0.321626
  cam1 SetViewAngle 21.4286
  cam1 SetViewUp -0.0136986 0.999858 0.00984497
ren1 SetActiveCamera cam1
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
