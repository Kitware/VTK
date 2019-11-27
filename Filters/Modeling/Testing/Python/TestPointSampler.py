#!/usr/bin/env python
import vtk

# Create the RenderWindow, Renderer and interactive renderer
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic polydata with two triangles, a quad, a general polygon,
# and a triangle strip. The polydata also has point attributes.
#
pts = vtk.vtkPoints()
pts.SetNumberOfPoints(11)
pts.SetPoint(0, 0.0,0.0,0.0)
pts.SetPoint(1, 1.0,0.0,0.0)
pts.SetPoint(2, 0.0,1.0,0.0)
pts.SetPoint(3, 1.0,1.0,0.0)
pts.SetPoint(4, 0.0,2.0,0.0)
pts.SetPoint(5, 1.0,2.0,0.0)
pts.SetPoint(6, 0.0,3.0,0.0)
pts.SetPoint(7, 1.0,3.0,0.0)
pts.SetPoint(8, 0.5,3.5,0.0)
pts.SetPoint(9, 0.0,-1,0.0)
pts.SetPoint(10, 1.0,-1,0.0)

cells = vtk.vtkCellArray()
cells.InsertNextCell(3)
cells.InsertCellPoint(0)
cells.InsertCellPoint(1)
cells.InsertCellPoint(2)
cells.InsertNextCell(3)
cells.InsertCellPoint(2)
cells.InsertCellPoint(1)
cells.InsertCellPoint(3)
cells.InsertNextCell(4)
cells.InsertCellPoint(2)
cells.InsertCellPoint(3)
cells.InsertCellPoint(5)
cells.InsertCellPoint(4)
cells.InsertNextCell(5)
cells.InsertCellPoint(4)
cells.InsertCellPoint(5)
cells.InsertCellPoint(7)
cells.InsertCellPoint(8)
cells.InsertCellPoint(6)

strip = vtk.vtkCellArray()
strip.InsertNextCell(4)
strip.InsertCellPoint(10)
strip.InsertCellPoint(1)
strip.InsertCellPoint(9)
strip.InsertCellPoint(0)

scalars = vtk.vtkFloatArray()
scalars.SetNumberOfTuples(11)
scalars.SetTuple1(0,0.0)
scalars.SetTuple1(1,1.0)
scalars.SetTuple1(2,0.0)
scalars.SetTuple1(3,1.0)
scalars.SetTuple1(4,0.0)
scalars.SetTuple1(5,1.0)
scalars.SetTuple1(6,0.0)
scalars.SetTuple1(7,1.0)
scalars.SetTuple1(8,0.5)
scalars.SetTuple1(9,0.0)
scalars.SetTuple1(10,1.0)

# Assemble the polydata
pd = vtk.vtkPolyData()
pd.SetPoints(pts)
pd.SetPolys(cells)
pd.SetStrips(strip)
pd.GetPointData().SetScalars(scalars)

# Finally test the point sampler
sampler = vtk.vtkPolyDataPointSampler()
sampler.SetInputData(pd)
sampler.SetDistance(0.05)
sampler.InterpolatePointDataOn()

# Mapper and actor
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(sampler.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Setup graphics stuff
ren1.AddActor(actor)
ren1.SetBackground(0,0,0)

renWin.SetSize(300,300)
renWin.Render()
iren.Start()
# --- end of script --
