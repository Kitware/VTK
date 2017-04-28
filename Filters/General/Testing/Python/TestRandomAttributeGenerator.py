#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and interactive renderer
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)

# make sure to have the same regression image on all platforms.
renWin.SetMultiSamples(0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Force a starting random value
raMath = vtk.vtkMath()
raMath.RandomSeed(6)

# Generate random attributes on a plane
#
ps = vtk.vtkPlaneSource()
ps.SetXResolution(10)
ps.SetYResolution(10)

ag = vtk.vtkRandomAttributeGenerator()
ag.SetInputConnection(ps.GetOutputPort())
ag.GenerateAllDataOn()

ss = vtk.vtkSphereSource()
ss.SetPhiResolution(16)
ss.SetThetaResolution(32)

tg = vtk.vtkTensorGlyph()
tg.SetInputConnection(ag.GetOutputPort())
tg.SetSourceConnection(ss.GetOutputPort())
tg.SetScaleFactor(0.1)
tg.SetMaxScaleFactor(10)
tg.ClampScalingOn()

n = vtk.vtkPolyDataNormals()
n.SetInputConnection(tg.GetOutputPort())

pdm = vtk.vtkPolyDataMapper()
pdm.SetInputConnection(n.GetOutputPort())

a = vtk.vtkActor()
a.SetMapper(pdm)

pm = vtk.vtkPolyDataMapper()
pm.SetInputConnection(ps.GetOutputPort())

pa = vtk.vtkActor()
pa.SetMapper(pm)

ren1.AddActor(a)
ren1.AddActor(pa)
ren1.SetBackground(0, 0, 0)

renWin.SetSize(300, 300)

renWin.Render()

#iren.Start()
