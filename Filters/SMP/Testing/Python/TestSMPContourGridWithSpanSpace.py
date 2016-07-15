#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Debugging parameters
sze = 300

reader = vtk.vtkExodusIIReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/disk_out_ref.ex2")
reader.UpdateInformation()
reader.SetPointResultArrayStatus("CH4", 1)
reader.Update()

tree = vtk.vtkSpanSpace()

#contour = vtk.vtkSMPContourGrid()
contour = vtk.vtkContourGrid()
contour.SetInputConnection(reader.GetOutputPort())
contour.SetInputArrayToProcess(0, 0, 0, vtk.vtkAssignAttribute.POINT_DATA, "CH4")
contour.SetValue(0, 0.000718448)
#contour.MergePiecesOff()
contour.UseScalarTreeOn()
contour.SetScalarTree(tree)
contour.Update()

mapper = vtk.vtkCompositePolyDataMapper2()
mapper.SetInputConnection(contour.GetOutputPort())
mapper.ScalarVisibilityOff()

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Create graphics stuff
#
ren1 = vtk.vtkRenderer()
ren1.AddActor(actor)
ren1.ResetCamera()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(sze, sze)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

renWin.Render()
#iren.Start()
