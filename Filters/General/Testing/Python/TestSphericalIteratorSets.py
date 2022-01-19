#!/usr/bin/env python
# -*- coding: utf-8 -*-
import vtk
from vtkmodules.vtkCommonCore import reference

# Test the generate of pre-defined vtkSphericalPointIterator axes sets.

# Generate a random point cloud as a placeholder
ps = vtk.vtkPointSource()
ps.SetNumberOfPoints(100)
ps.SetCenter(0,0,0)
ps.SetRadius(10)
ps.Update()

# Create several iterators with different axes sets

XY_CW_AXES = 0
pd0 = vtk.vtkPolyData()
pcenter = [0,0,0]
piter0 = vtk.vtkSphericalPointIterator()
piter0.SetDataSet(ps.GetOutput())
piter0.SetAxes(XY_CW_AXES,6)
piter0.SetSortTypeToNone()
piter0.Initialize(pcenter)
piter0.BuildRepresentation(pd0)

# Render the axes points
aMapper0 = vtk.vtkPolyDataMapper()
aMapper0.SetInputData(pd0)

aActor0 = vtk.vtkActor()
aActor0.SetMapper(aMapper0)
aActor0.GetProperty().SetColor(0.85,0.85,0.85)
aActor0.GetProperty().SetLineWidth(2)

XY_CCW_AXES = 1
pd1 = vtk.vtkPolyData()
pcenter = [2.5,0,0]
piter1 = vtk.vtkSphericalPointIterator()
piter1.SetDataSet(ps.GetOutput())
piter1.SetAxes(XY_CCW_AXES,6)
piter1.SetSortTypeToNone()
piter1.Initialize(pcenter)
piter1.BuildRepresentation(pd1)

# Render the axes points
aMapper1 = vtk.vtkPolyDataMapper()
aMapper1.SetInputData(pd1)

aActor1 = vtk.vtkActor()
aActor1.SetMapper(aMapper1)
aActor1.GetProperty().SetColor(0.85,0.85,0.85)
aActor1.GetProperty().SetLineWidth(2)

XY_SQUARE_AXES = 2
pd2 = vtk.vtkPolyData()
pcenter = [5,0,0]
piter2 = vtk.vtkSphericalPointIterator()
piter2.SetDataSet(ps.GetOutput())
piter2.SetAxes(XY_SQUARE_AXES)
piter2.SetSortTypeToNone()
piter2.Initialize(pcenter)
piter2.BuildRepresentation(pd2)

# Render the axes points
aMapper2 = vtk.vtkPolyDataMapper()
aMapper2.SetInputData(pd2)

aActor2 = vtk.vtkActor()
aActor2.SetMapper(aMapper2)
aActor2.GetProperty().SetColor(0.85,0.85,0.85)
aActor2.GetProperty().SetLineWidth(2)

CUBE_AXES = 3
pd3 = vtk.vtkPolyData()
pcenter = [7.5,0,0]
piter3 = vtk.vtkSphericalPointIterator()
piter3.SetDataSet(ps.GetOutput())
piter3.SetAxes(CUBE_AXES)
piter3.SetSortTypeToNone()
piter3.Initialize(pcenter)
piter3.BuildRepresentation(pd3)

# Render the axes points
aMapper3 = vtk.vtkPolyDataMapper()
aMapper3.SetInputData(pd3)

aActor3 = vtk.vtkActor()
aActor3.SetMapper(aMapper3)
aActor3.GetProperty().SetColor(0.85,0.85,0.85)
aActor3.GetProperty().SetLineWidth(2)

OCTAHEDRON_AXES = 4
pd4 = vtk.vtkPolyData()
pcenter = [0,2.5,0]
piter4 = vtk.vtkSphericalPointIterator()
piter4.SetDataSet(ps.GetOutput())
piter4.SetAxes(OCTAHEDRON_AXES)
piter4.SetSortTypeToNone()
piter4.Initialize(pcenter)
piter4.BuildRepresentation(pd4)

# Render the axes points
aMapper4 = vtk.vtkPolyDataMapper()
aMapper4.SetInputData(pd4)

aActor4 = vtk.vtkActor()
aActor4.SetMapper(aMapper4)
aActor4.GetProperty().SetColor(0.85,0.85,0.85)
aActor4.GetProperty().SetLineWidth(2)

CUBE_OCTAHEDRON_AXES = 5
pd5 = vtk.vtkPolyData()
pcenter = [2.5,2.5,0]
piter5 = vtk.vtkSphericalPointIterator()
piter5.SetDataSet(ps.GetOutput())
piter5.SetAxes(CUBE_OCTAHEDRON_AXES)
piter5.SetSortTypeToNone()
piter5.Initialize(pcenter)
piter5.BuildRepresentation(pd5)

# Render the axes points
aMapper5 = vtk.vtkPolyDataMapper()
aMapper5.SetInputData(pd5)

aActor5 = vtk.vtkActor()
aActor5.SetMapper(aMapper5)
aActor5.GetProperty().SetColor(0.85,0.85,0.85)
aActor5.GetProperty().SetLineWidth(2)

DODECAHEDRON_AXES = 6
pd6 = vtk.vtkPolyData()
pcenter = [5,2.5,0]
piter6 = vtk.vtkSphericalPointIterator()
piter6.SetDataSet(ps.GetOutput())
piter6.SetAxes(DODECAHEDRON_AXES)
piter6.SetSortTypeToNone()
piter6.Initialize(pcenter)
piter6.BuildRepresentation(pd6)

# Render the axes points
aMapper6 = vtk.vtkPolyDataMapper()
aMapper6.SetInputData(pd6)

aActor6 = vtk.vtkActor()
aActor6.SetMapper(aMapper6)
aActor6.GetProperty().SetColor(0.85,0.85,0.85)
aActor6.GetProperty().SetLineWidth(2)

ICOSAHEDRON_AXES = 7
pd7 = vtk.vtkPolyData()
pcenter = [7.5,2.5,0]
piter7 = vtk.vtkSphericalPointIterator()
piter7.SetDataSet(ps.GetOutput())
piter7.SetAxes(ICOSAHEDRON_AXES)
piter7.SetSortTypeToNone()
piter7.Initialize(pcenter)
piter7.BuildRepresentation(pd7)

# Render the axes points
aMapper7 = vtk.vtkPolyDataMapper()
aMapper7.SetInputData(pd7)

aActor7 = vtk.vtkActor()
aActor7.SetMapper(aMapper7)
aActor7.GetProperty().SetColor(0.85,0.85,0.85)
aActor7.GetProperty().SetLineWidth(2)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)

iRen = vtk.vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(aActor0)
ren0.AddActor(aActor1)
ren0.AddActor(aActor2)
ren0.AddActor(aActor3)
ren0.AddActor(aActor4)
ren0.AddActor(aActor5)
ren0.AddActor(aActor6)
ren0.AddActor(aActor7)
ren0.GetActiveCamera().SetFocalPoint(1.25,3.75,0)
ren0.GetActiveCamera().SetPosition(1.45,3.85,.4)

renWin.SetSize(400, 200)

iRen.Initialize()
ren0.ResetCamera()
ren0.GetActiveCamera().Zoom(1.65)
renWin.Render()

# Interact with the data
iRen.Start()
