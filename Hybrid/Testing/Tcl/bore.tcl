package require vtk
package require vtkinteraction


# Create arc plots

# get the interactor ui

vtkCamera camera

# read the bore
vtkPolyDataReader bore
    bore SetFileName "$VTK_DATA_ROOT/Data/bore.vtk"
vtkTubeFilter tuber
    tuber SetInput [bore GetOutput]
    tuber SetNumberOfSides 6
    tuber SetRadius 15
vtkPolyDataMapper mapBore
    mapBore SetInput [tuber GetOutput]
    mapBore ScalarVisibilityOff
vtkActor boreActor
    boreActor SetMapper mapBore
    [boreActor GetProperty] SetColor 0 0 0

# create the arc plots
#
vtkPolyDataReader track1
    track1 SetFileName "$VTK_DATA_ROOT/Data/track1.binary.vtk"
vtkArcPlotter ap
    ap SetInput [track1 GetOutput]
    ap SetCamera camera
    ap SetRadius 250.0
    ap SetHeight 200.0
    ap UseDefaultNormalOn
    ap SetDefaultNormal 1 1 0
vtkPolyDataMapper mapArc
    mapArc SetInput [ap GetOutput]
vtkActor arcActor
    arcActor SetMapper mapArc
    [arcActor GetProperty] SetColor 0 1 0

vtkPolyDataReader track2
    track2 SetFileName "$VTK_DATA_ROOT/Data/track2.binary.vtk"
vtkArcPlotter ap2
    ap2 SetInput [track2 GetOutput]
    ap2 SetCamera camera
    ap2 SetRadius 450.0
    ap2 SetHeight 200.0
    ap2 UseDefaultNormalOn
    ap2 SetDefaultNormal 1 1 0
vtkPolyDataMapper mapArc2
    mapArc2 SetInput [ap2 GetOutput]
vtkActor arcActor2
    arcActor2 SetMapper mapArc2
    [arcActor2 GetProperty] SetColor 0 0 1

vtkPolyDataReader track3
    track3 SetFileName "$VTK_DATA_ROOT/Data/track3.binary.vtk"
vtkArcPlotter ap3
    ap3 SetInput [track3 GetOutput]
    ap3 SetCamera camera
    ap3 SetRadius 250.0
    ap3 SetHeight 50.0
    ap3 SetDefaultNormal 1 1 0
vtkPolyDataMapper mapArc3
    mapArc3 SetInput [ap3 GetOutput]
vtkActor arcActor3
    arcActor3 SetMapper mapArc3
    [arcActor3 GetProperty] SetColor 1 0 1

# Create graphics objects
# Create the rendering window  renderer  and interactive renderer
vtkRenderer ren1
    ren1 SetActiveCamera camera
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer  set the background and size
ren1 AddActor boreActor
ren1 AddActor arcActor
ren1 AddActor arcActor2
ren1 AddActor arcActor3

ren1 SetBackground 1 1 1
renWin SetSize 235 500

camera SetClippingRange 14144 32817
camera SetFocalPoint -1023 680 5812
camera SetPosition 15551 -2426 19820
camera SetViewUp -0.651889 -0.07576 0.754521
camera SetViewAngle 20

renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


