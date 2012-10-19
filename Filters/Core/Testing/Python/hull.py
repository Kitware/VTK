#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Generate random planes to form a convex polyhedron.
# Create a polyhedral representation of the planes.
# get the interactor ui
# create some points laying between 1<=r<5 (r is radius)
# the points also have normals pointing away from the origin.
#
mathObj = vtk.vtkMath()
points = vtk.vtkPoints()
normals = vtk.vtkFloatArray()
normals.SetNumberOfComponents(3)
i = 0
while i < 100:
    radius = 1.0
    theta = mathObj.Random(0,360)
    phi = mathObj.Random(0,180)
    x = expr.expr(globals(), locals(),["radius","*","sin","(","phi",")*","cos","(","theta",")"])
    y = expr.expr(globals(), locals(),["radius","*","sin","(","phi",")*","sin","(","theta",")"])
    z = expr.expr(globals(), locals(),["radius","*","cos","(","phi",")"])
    points.InsertPoint(i,x,y,z)
    normals.InsertTuple3(i,x,y,z)
    i = i + 1

planes = vtk.vtkPlanes()
planes.SetPoints(points)
planes.SetNormals(normals)
ss = vtk.vtkSphereSource()
hull = vtk.vtkHull()
hull.SetPlanes(planes)
pd = vtk.vtkPolyData()
hull.GenerateHull(pd,-20,20,-20,20,-20,20)
# triangulate them
#
mapHull = vtk.vtkPolyDataMapper()
mapHull.SetInputData(pd)
hullActor = vtk.vtkActor()
hullActor.SetMapper(mapHull)
# Create graphics objects
# Create the rendering window, renderer, and interactive renderer
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
ren1.AddActor(hullActor)
renWin.SetSize(250,250)
# render the image
#
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(1.5)
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
