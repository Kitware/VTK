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
    gf SetInput [pl3d GetOutput]
vtkPolyDataMapper gMapper
    gMapper SetInput [gf GetOutput]
    eval gMapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor gActor
    gActor SetMapper gMapper
vtkDataSetSurfaceFilter gf2
    gf2 SetInput [pl3d GetOutput]
    gf2 UseStripsOn
vtkPolyDataMapper g2Mapper
    g2Mapper SetInput [gf2 GetOutput]
    eval g2Mapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor g2Actor
    g2Actor SetMapper g2Mapper
    g2Actor AddPosition 0 15 0

# create pipeline - poly data
#
vtkDataSetSurfaceFilter gf3
    gf3 SetInput [gf GetOutput]
vtkPolyDataMapper g3Mapper
    g3Mapper SetInput [gf3 GetOutput]
    eval g3Mapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor g3Actor
    g3Actor SetMapper g3Mapper
    g3Actor AddPosition 0 0 15
vtkDataSetSurfaceFilter gf4
    gf4 SetInput [gf2 GetOutput]
    gf4 UseStripsOn
vtkPolyDataMapper g4Mapper
    g4Mapper SetInput [gf4 GetOutput]
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
    eg SetInput [pl3d GetOutput]
    eg SetImplicitFunction s
vtkDataSetSurfaceFilter gf5
    gf5 SetInput [eg GetOutput]
vtkPolyDataMapper g5Mapper
    g5Mapper SetInput [gf5 GetOutput]
    eval g5Mapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor g5Actor
    g5Actor SetMapper g5Mapper
    g5Actor AddPosition 0 0 30
vtkDataSetSurfaceFilter gf6
    gf6 SetInput [eg GetOutput]
    gf6 UseStripsOn
vtkPolyDataMapper g6Mapper
    g6Mapper SetInput [gf6 GetOutput]
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
    gf7 SetInput [rgridReader GetOutput]
vtkPolyDataMapper g7Mapper
    g7Mapper SetInput [gf7 GetOutput]
    eval g7Mapper SetScalarRange [[rgridReader GetOutput] GetScalarRange]
vtkActor g7Actor
    g7Actor SetMapper g7Mapper
    g7Actor SetScale 3 3 3
vtkDataSetSurfaceFilter gf8
    gf8 SetInput [rgridReader GetOutput]
    gf8 UseStripsOn
vtkPolyDataMapper g8Mapper
    g8Mapper SetInput [gf8 GetOutput]
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


