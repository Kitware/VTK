#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def PlaneSphereActors():
    ps = vtk.vtkPlaneSource()
    ps.SetXResolution(10)
    ps.SetYResolution(10)

    ss = vtk.vtkSphereSource()
    ss.SetRadius (0.3)

    group = vtk.vtkMultiBlockDataGroupFilter()
    group.AddInputConnection(ps.GetOutputPort())
    group.AddInputConnection(ss.GetOutputPort())

    ag = vtk.vtkRandomAttributeGenerator()
    ag.SetInputConnection(group.GetOutputPort())
    ag.GenerateCellScalarsOn()
    ag.AttributesConstantPerBlockOn()

    n = vtk.vtkPolyDataNormals()
    n.SetInputConnection(ag.GetOutputPort())
    n.Update ();

    actors = []
    it = n.GetOutputDataObject(0).NewIterator()
    it.InitTraversal()
    while not it.IsDoneWithTraversal():
        pm = vtk.vtkPolyDataMapper()
        pm.SetInputData(it.GetCurrentDataObject())

        a = vtk.vtkActor()
        a.SetMapper(pm)
        actors.append (a)
        it.GoToNextItem()
    return actors

# Create the RenderWindow, Renderer and interactive renderer
ren = vtk.vtkRenderer()
ren.SetBackground(0, 0, 0)
renWin = vtk.vtkRenderWindow()
renWin.SetSize(300, 300)
renWin.AddRenderer(ren)

# make sure to have the same regression image on all platforms.
renWin.SetMultiSamples(0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Force a starting random value
raMath = vtk.vtkMath()
raMath.RandomSeed(6)

# Generate random cell attributes on a plane and a cylinder
for a in PlaneSphereActors():
    ren.AddActor(a)

# reorient the camera
camera = ren.GetActiveCamera()
camera.Azimuth(20)
camera.Elevation(20)
ren.SetActiveCamera(camera)
ren.ResetCamera()

renWin.Render()
#iren.Start()
