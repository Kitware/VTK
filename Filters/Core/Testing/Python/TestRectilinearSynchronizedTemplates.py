#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create pipeline - rectilinear grid
#
rgridReader = vtk.vtkRectilinearGridReader()
rgridReader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/RectGrid2.vtk")
rgridReader.Update()
contour = vtk.vtkRectilinearSynchronizedTemplates()
contour.SetInputConnection(rgridReader.GetOutputPort())
contour.SetValue(0,1)
contour.ComputeScalarsOff()
contour.ComputeNormalsOn()
contour.ComputeGradientsOn()
cMapper = vtk.vtkPolyDataMapper()
cMapper.SetInputConnection(contour.GetOutputPort())
cActor = vtk.vtkActor()
cActor.SetMapper(cMapper)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(200,200)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(cActor)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
