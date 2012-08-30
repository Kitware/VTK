#!/usr/bin/env python

disk = vtk.vtkDiskSource()
disk.SetRadialResolution(2)
disk.SetCircumferentialResolution(9)
clean = vtk.vtkCleanPolyData()
clean.SetInputConnection(disk.GetOutputPort())
clean.SetTolerance(0.01)
piece = vtk.vtkExtractPolyDataPiece()
piece.SetInputConnection(clean.GetOutputPort())
extrude = vtk.vtkPLinearExtrusionFilter()
extrude.SetInputConnection(piece.GetOutputPort())
extrude.PieceInvariantOn()
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(extrude.GetOutputPort())
mapper.SetNumberOfPieces(2)
mapper.SetPiece(1)
bf = vtk.vtkProperty()
bf.SetColor(1,0,0)
actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,1,0.8)
actor.SetBackfaceProperty(bf)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)
# render the image
#
cam1 = ren1.GetActiveCamera()
cam1.Azimuth(20)
cam1.Elevation(40)
ren1.ResetCamera()
cam1.Zoom(1.2)
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
