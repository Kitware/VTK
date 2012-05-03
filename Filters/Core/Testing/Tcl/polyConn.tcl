package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# read data
#
vtkMultiBlockPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
    set output [[pl3d GetOutput] GetBlock 0]

# planes to connect
vtkStructuredGridGeometryFilter plane1
    plane1 SetInputData $output
    plane1 SetExtent 20 20 0 100 0 100
vtkPolyDataConnectivityFilter conn
    conn SetInputConnection [plane1 GetOutputPort]
    conn ScalarConnectivityOn
    conn SetScalarRange 0.19 0.25
    conn Update
    conn Print

vtkPolyDataMapper plane1Map
    plane1Map SetInputConnection [conn GetOutputPort]
    eval plane1Map SetScalarRange [$output GetScalarRange]
vtkActor plane1Actor
    plane1Actor SetMapper plane1Map
    [plane1Actor GetProperty] SetOpacity 0.999

# outline
vtkStructuredGridOutlineFilter outline
    outline SetInputData $output
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    set outlineProp [outlineActor GetProperty]
    eval $outlineProp SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor plane1Actor
ren1 SetBackground 1 1 1
renWin SetSize 300 300

vtkCamera cam1
  cam1 SetClippingRange 14.29 63.53
  cam1 SetFocalPoint 8.58522 1.58266 30.6486
  cam1 SetPosition 37.6808 -20.1298 35.4016
  cam1 SetViewAngle 30
  cam1 SetViewUp -0.0566235 0.140504 0.98846
ren1 SetActiveCamera cam1

iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



