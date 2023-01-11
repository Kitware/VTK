#!/usr/bin/env python
# -*- coding: utf-8 -*-
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
    vtkSphericalPointIterator,
)
from vtkmodules.vtkFiltersPoints import vtkProjectPointsToPlane
from vtkmodules.vtkFiltersSources import vtkPointSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.vtkCommonCore import reference

# Iterate over points in a variety of ways using a
# vtkSphericalPointIterator

# Control the size of the test
res = 250
numSweeps = 3

# Generate a random point cloud
ps = vtkPointSource()
ps.SetNumberOfPoints(res)
ps.SetCenter(0,0,0)
ps.SetRadius(10)

pc = vtkProjectPointsToPlane()
pc.SetInputConnection(ps.GetOutputPort())
pc.SetProjectionTypeToZPlane()
pc.Update()

# Create an iterator
XY_CW_AXES = 0
XY_CCW_AXES = 1
pcenter = [1,2,3]
piter = vtkSphericalPointIterator()
piter.SetDataSet(pc.GetOutput())
piter.SetAxes(XY_CW_AXES,40)
piter.SetSortTypeToAscending()
piter.Initialize(pcenter)

# Stash to collect axis sweeps
pd = vtkPolyData()
pd.SetPoints(pc.GetOutput().GetPoints())
ca = vtkCellArray()
pd.SetVerts(ca)

# Render the axes points
aMapper = vtkPolyDataMapper()
aMapper.SetInputData(pd)

aActor = vtkActor()
aActor.SetMapper(aMapper)
aActor.GetProperty().SetColor(0.85,0.85,0.85)
aActor.GetProperty().SetPointSize(2)

# Spiral iterate over point cloud
# Stash to collect axis sweeps
pd2 = vtkPolyData()
pd2.SetPoints(pc.GetOutput().GetPoints())
ca2 = vtkCellArray()
pd2.SetVerts(ca2)

# Render the axes points
sMapper = vtkPolyDataMapper()
sMapper.SetInputData(pd2)

sActor = vtkActor()
sActor.SetMapper(sMapper)
sActor.GetProperty().SetColor(0.85,0.85,0.85)
sActor.GetProperty().SetPointSize(2)

# Render the point cloud
pcMapper = vtkPolyDataMapper()
pcMapper.SetInputConnection(pc.GetOutputPort())

pcActor = vtkActor()
pcActor.SetMapper(pcMapper)
pcActor.GetProperty().SetColor(0.85,0.5,0.5)
pcActor.GetProperty().SetPointSize(1)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
ren0.SetViewport(0, 0, 0.5, 1)
ren1 = vtkRenderer()
ren1.SetViewport(0.5, 0, 1, 1)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)

iRen = vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(aActor)
ren0.AddActor(pcActor)
ren0.SetBackground(0,0,0)
ren0.GetActiveCamera().SetFocalPoint(0,0,0)
ren0.GetActiveCamera().SetPosition(0,0,1)

ren1.AddActor(sActor)
ren1.AddActor(pcActor)
ren1.SetBackground(0,0,0)
ren1.GetActiveCamera().SetFocalPoint(0,0,0)
ren1.GetActiveCamera().SetPosition(0,0,1)

renWin.SetSize(400, 200)

iRen.Initialize()
ren0.ResetCamera()
ren1.ResetCamera()
renWin.Render()

# Create a lighthouse effect by looping over all axes
npts = reference(0)
pts = reference((0,))
numAxes = piter.GetNumberOfAxes()

for sweeps in range(0,numSweeps):
    for i in range(0,numAxes):
        piter.GetAxisPoints(i,npts,pts)
        ca.Reset()
        ca.InsertNextCell(npts)
        for i in range(0,npts):
            ca.InsertCellPoint(pts[i])
        pd.Modified()
        renWin.Render()

# Spiral out from the center
pIds = [0]
ca.Reset()
piter.GoToFirstPoint()
while not piter.IsDoneWithTraversal():
    pIds[0] = piter.GetCurrentPoint()
    ca2.InsertNextCell(1,pIds)
    piter.GoToNextPoint()
    pd2.Modified()
    renWin.Render()

# Interact with the data
iRen.Start()
