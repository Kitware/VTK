#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create a pipeline: some skinny-ass triangles
sphere = vtk.vtkSphereSource()
sphere.SetThetaResolution(6)
sphere.SetPhiResolution(24)

ids = vtk.vtkIdFilter()
ids.SetInputConnection(sphere.GetOutputPort())

adapt = vtk.vtkAdaptiveSubdivisionFilter()
adapt.SetInputConnection(ids.GetOutputPort())
adapt.SetMaximumEdgeLength(0.1)
#adapt.SetMaximumTriangleArea(0.01)
#adapt.SetMaximumNumberOfPasses(1)
adapt.Update()

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(adapt.GetOutputPort())
mapper.SetScalarModeToUseCellFieldData()
mapper.SelectColorArray("vtkIdFilter_Ids")
mapper.SetScalarRange(adapt.GetOutput().GetCellData().GetScalars().GetRange())

edgeProp = vtk.vtkProperty()
edgeProp.EdgeVisibilityOn()
edgeProp.SetEdgeColor(0,0,0)

actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.SetProperty(edgeProp)

# Graphics stuff
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(0,0,0)
ren1.GetActiveCamera().SetFocalPoint(0,0,0)
ren1.GetActiveCamera().SetPosition(0,0,1)
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(1.5)

renWin.SetSize(300,300)
renWin.Render()
#iren.Start()
