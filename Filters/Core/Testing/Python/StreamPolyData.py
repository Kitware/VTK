#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

NUMBER_OF_PIECES = 5
# Generate implicit model of a sphere
#
# Create renderer stuff
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline that handles ghost cells
sphere = vtk.vtkSphereSource()
sphere.SetRadius(3)
sphere.SetPhiResolution(100)
sphere.SetThetaResolution(150)
# sphere AddObserver StartEvent {tk_messageBox -message "Executing with piece [[sphere GetOutput] GetUpdatePiece]"}
# Just playing with an alternative that is not currently used.
def NotUsed (__vtk__temp0=0,__vtk__temp1=0):
    # This filter actually spoils the example because it asks for the whole input.
    # The only reason it is here is because sphere complains it cannot generate ghost cells.
    vtkExtractPolyDataPiece.piece()
    piece.SetInputConnection(sphere.GetOutputPort())
    # purposely put seams in here.
    piece.CreateGhostCellsOff()
    # purposely put seams in here.
    pdn = vtk.vtkPolyDataNormals()
    pdn.SetInputConnection(piece.GetOutputPort())

# Just playing with an alternative that is not currently used.
deci = vtk.vtkDecimatePro()
deci.SetInputConnection(sphere.GetOutputPort())
# this did not remove seams as I thought it would
deci.BoundaryVertexDeletionOff()
#deci PreserveTopologyOn
# Since quadric Clustering does not handle borders properly yet,
# the pieces will have dramatic "eams"
q = vtk.vtkQuadricClustering()
q.SetInputConnection(sphere.GetOutputPort())
q.SetNumberOfXDivisions(5)
q.SetNumberOfYDivisions(5)
q.SetNumberOfZDivisions(10)
q.UseInputPointsOn()
streamer = vtk.vtkPolyDataStreamer()
#streamer SetInputConnection [deci GetOutputPort]
streamer.SetInputConnection(q.GetOutputPort())
#streamer SetInputConnection [pdn GetOutputPort]
streamer.SetNumberOfStreamDivisions(NUMBER_OF_PIECES)
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(streamer.GetOutputPort())
mapper.ScalarVisibilityOff()
mapper.SetPiece(0)
mapper.SetNumberOfPieces(2)
mapper.ImmediateModeRenderingOn()
actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(english_red)
# Add the actors to the renderer, set the background and size
#
ren1.GetActiveCamera().SetPosition(5,5,10)
ren1.GetActiveCamera().SetFocalPoint(0,0,0)
ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
