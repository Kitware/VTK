package require vtktcl_interactor
::vtktcl::load_script colors

# Create a simple graph (it's jittered from optimum)
vtkPoints pts
pts SetNumberOfPoints 10
pts SetPoint 0 -0.5  1.0 -0.3
pts SetPoint 1 -3.0  0.1  0.2
pts SetPoint 2  0.0  0.0  0.0
pts SetPoint 3  1.2 -0.1 -0.2
pts SetPoint 4  0.2 -3.0  0.2
pts SetPoint 5 -4.2 -5.5  0.7
pts SetPoint 6  1.2 -7.3 -0.6
pts SetPoint 7  4.2 -5.5  0.7
pts SetPoint 8  0.0  0.0 -0.4
pts SetPoint 9  0.0  0.0  0.8

vtkCellArray lines
lines InsertNextCell 4
lines InsertCellPoint 0
lines InsertCellPoint 2
lines InsertCellPoint 4
lines InsertCellPoint 6
lines InsertNextCell 2
lines InsertCellPoint 1
lines InsertCellPoint 2
lines InsertNextCell 2
lines InsertCellPoint 2
lines InsertCellPoint 3
lines InsertNextCell 2
lines InsertCellPoint 5
lines InsertCellPoint 6
lines InsertNextCell 2
lines InsertCellPoint 6
lines InsertCellPoint 7
lines InsertNextCell 2
lines InsertCellPoint 2
lines InsertCellPoint 8
lines InsertNextCell 2
lines InsertCellPoint 2
lines InsertCellPoint 9

vtkPolyData pd
pd SetPoints pts
pd SetLines lines

vtkGraphLayoutFilter layout
layout SetInput pd
layout ThreeDimensionalLayoutOn

vtkTubeFilter tubes
    tubes SetInput [layout GetOutput]
    tubes SetRadius 0.1
    tubes SetNumberOfSides 6
vtkPolyDataMapper mapEdges
    mapEdges SetInput [tubes GetOutput]
vtkActor edgeActor
    edgeActor SetMapper mapEdges
    eval [edgeActor GetProperty] SetColor $peacock
    [edgeActor GetProperty] SetSpecularColor 1 1 1
    [edgeActor GetProperty] SetSpecular 0.3
    [edgeActor GetProperty] SetSpecularPower 20
    [edgeActor GetProperty] SetAmbient 0.2
    [edgeActor GetProperty] SetDiffuse 0.8

vtkSphereSource ball
    ball SetRadius 0.25
    ball SetThetaResolution 12
    ball SetPhiResolution 12
vtkGlyph3D balls
    balls SetInput [layout GetOutput]
    balls SetSource [ball GetOutput]
vtkPolyDataMapper mapBalls
    mapBalls SetInput [balls GetOutput]
vtkActor ballActor
    ballActor SetMapper mapBalls
    eval [ballActor GetProperty] SetColor $hot_pink
    [ballActor GetProperty] SetSpecularColor 1 1 1
    [ballActor GetProperty] SetSpecular 0.3
    [ballActor GetProperty] SetSpecularPower 20
    [ballActor GetProperty] SetAmbient 0.2
    [ballActor GetProperty] SetDiffuse 0.8

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor edgeActor
ren1 AddActor ballActor

ren1 SetBackground 1 1 1
renWin SetSize 250 250

set cam1 [ren1 GetActiveCamera]
    $cam1 SetClippingRange 19.245 47.25
    $cam1 SetFocalPoint 80.1472 169.291 -279.609
    $cam1 SetPosition 58.1543 192.094 -276.643
    $cam1 SetViewUp 0.404855 0.490812 -0.77149

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
