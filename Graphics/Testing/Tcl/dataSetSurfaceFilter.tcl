package require vtk
package require vtkinteraction

# create pipeline - structured grid
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

vtkDataSetSurfaceFilter gf
    gf SetInputConnection [pl3d GetOutputPort]
vtkPolyDataMapper gMapper
    gMapper SetInputConnection [gf GetOutputPort]
    eval gMapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor gActor
    gActor SetMapper gMapper
vtkDataSetSurfaceFilter gf2
    gf2 SetInputConnection [pl3d GetOutputPort]
    gf2 UseStripsOn
vtkPolyDataMapper g2Mapper
    g2Mapper SetInputConnection [gf2 GetOutputPort]
    eval g2Mapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor g2Actor
    g2Actor SetMapper g2Mapper
    g2Actor AddPosition 0 15 0

# create pipeline - poly data
#
vtkDataSetSurfaceFilter gf3
    gf3 SetInputConnection [gf GetOutputPort]
vtkPolyDataMapper g3Mapper
    g3Mapper SetInputConnection [gf3 GetOutputPort]
    eval g3Mapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor g3Actor
    g3Actor SetMapper g3Mapper
    g3Actor AddPosition 0 0 15
vtkDataSetSurfaceFilter gf4
    gf4 SetInputConnection [gf2 GetOutputPort]
    gf4 UseStripsOn
vtkPolyDataMapper g4Mapper
    g4Mapper SetInputConnection [gf4 GetOutputPort]
    eval g4Mapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor g4Actor
    g4Actor SetMapper g4Mapper
    g4Actor AddPosition 0 15 15

# create pipeline - unstructured grid
#
vtkSphere s
    eval s SetCenter [[pl3d GetOutput] GetCenter]
    s SetRadius 100.0; #everything
vtkExtractGeometry eg
    eg SetInputConnection [pl3d GetOutputPort]
    eg SetImplicitFunction s
vtkDataSetSurfaceFilter gf5
    gf5 SetInputConnection [eg GetOutputPort]
vtkPolyDataMapper g5Mapper
    g5Mapper SetInputConnection [gf5 GetOutputPort]
    eval g5Mapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor g5Actor
    g5Actor SetMapper g5Mapper
    g5Actor AddPosition 0 0 30
vtkDataSetSurfaceFilter gf6
    gf6 SetInputConnection [eg GetOutputPort]
    gf6 UseStripsOn
vtkPolyDataMapper g6Mapper
    g6Mapper SetInputConnection [gf6 GetOutputPort]
    eval g6Mapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor g6Actor
    g6Actor SetMapper g6Mapper
    g6Actor AddPosition 0 15 30

# create pipeline - rectilinear grid
#
vtkRectilinearGridReader rgridReader
    rgridReader SetFileName "$VTK_DATA_ROOT/Data/RectGrid2.vtk"
    rgridReader Update
vtkDataSetSurfaceFilter gf7
    gf7 SetInputConnection [rgridReader GetOutputPort]
vtkPolyDataMapper g7Mapper
    g7Mapper SetInputConnection [gf7 GetOutputPort]
    eval g7Mapper SetScalarRange [[rgridReader GetOutput] GetScalarRange]
vtkActor g7Actor
    g7Actor SetMapper g7Mapper
    g7Actor SetScale 3 3 3
vtkDataSetSurfaceFilter gf8
    gf8 SetInputConnection [rgridReader GetOutputPort]
    gf8 UseStripsOn
vtkPolyDataMapper g8Mapper
    g8Mapper SetInputConnection [gf8 GetOutputPort]
    eval g8Mapper SetScalarRange [[rgridReader GetOutput] GetScalarRange]
vtkActor g8Actor
    g8Actor SetMapper g8Mapper
    g8Actor SetScale 3 3 3
    g8Actor AddPosition 0 15 0

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor gActor
ren1 AddActor g2Actor
ren1 AddActor g3Actor
ren1 AddActor g4Actor
ren1 AddActor g5Actor
ren1 AddActor g6Actor
ren1 AddActor g7Actor
ren1 AddActor g8Actor
renWin SetSize 340 550

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 84 174
$cam1 SetFocalPoint 5.22824 6.09412 35.9813
$cam1 SetPosition 100.052 62.875 102.818
$cam1 SetViewUp -0.307455 -0.464269 0.830617
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


