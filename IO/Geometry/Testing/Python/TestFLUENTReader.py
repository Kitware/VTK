#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Read some Fluent UCD data in ASCII form
r = vtk.vtkFLUENTReader()
r.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/room.cas")
r.EnableAllCellArrays()

g = vtk.vtkGeometryFilter()
g.SetInputConnection(r.GetOutputPort())

FluentMapper = vtk.vtkCompositePolyDataMapper2()
FluentMapper.SetInputConnection(g.GetOutputPort())
FluentMapper.SetScalarModeToUseCellFieldData()
FluentMapper.SelectColorArray("PRESSURE")
FluentMapper.SetScalarRange(-31, 44)
FluentActor = vtk.vtkActor()
FluentActor.SetMapper(FluentMapper)

ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(FluentActor)
renWin.SetSize(300,300)
renWin.Render()
ren1.ResetCamera()
ren1.GetActiveCamera().SetPosition(2, 2, 11)
ren1.GetActiveCamera().SetFocalPoint(2, 2, 0)
ren1.GetActiveCamera().SetViewUp(0, 1, 0)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.ResetCameraClippingRange()

iren.Initialize()
# --- end of script --
