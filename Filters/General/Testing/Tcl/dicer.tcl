package require vtk
package require vtkinteraction
package require vtktesting

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkSTLReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/42400-IDGH.stl"
vtkOBBDicer dicer
    dicer SetInputConnection [reader GetOutputPort]
    dicer SetNumberOfPointsPerPiece 1000
    dicer Update
vtkDataSetMapper isoMapper
    isoMapper SetInputConnection [dicer GetOutputPort]
    isoMapper SetScalarRange 0 [dicer GetNumberOfActualPieces]
vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetColor 0.7 0.3 0.3

vtkOutlineCornerFilter outline
    outline SetInputConnection [reader GetOutputPort]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    [outlineActor GetProperty] SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor isoActor
ren1 AddActor outlineActor
ren1 SetBackground 1 1 1
renWin SetSize 400 400
eval ren1 SetBackground 0.5 0.5 0.6

# render the image
#
renWin Render


# prevent the tk window from showing up then start the event loop
wm withdraw .

