#!/usr/bin/env python

# Decimate polyline with maximum error
import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create rendering stuff
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iRen = vtk.vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin);

# Plane source is used to create a polyline around boundary and then
# decimated.
ps = vtk.vtkPlaneSource()
ps.SetResolution(50,100)

fe = vtk.vtkFeatureEdges()
fe.SetInputConnection(ps.GetOutputPort())
fe.BoundaryEdgesOn()
fe.FeatureEdgesOff()
fe.NonManifoldEdgesOff()

s = vtk.vtkStripper()
s.SetInputConnection(fe.GetOutputPort())

deci = vtk.vtkDecimatePolylineFilter()
deci.SetInputConnection(s.GetOutputPort())
deci.SetMaximumError(0.00001)
deci.SetTargetReduction(0.99)
deci.Update()

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(deci.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(actor)

cam = ren.GetActiveCamera()
cam.SetPosition(0,0,1)
cam.SetFocalPoint(0,0,0)
ren.ResetCamera()

# render and interact with data
renWin.SetSize(300, 300)
renWin.Render()
iRen.Start()
