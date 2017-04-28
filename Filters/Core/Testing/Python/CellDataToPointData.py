#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# cell scalars to point scalars
# get the interactor ui
# Create the RenderWindow, Renderer and RenderWindowInteractor
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create a 2*2 cell/3*3 pt structuredgrid
points = vtk.vtkPoints()
points.InsertNextPoint(-1,1,0)
points.InsertNextPoint(0,1,0)
points.InsertNextPoint(1,1,0)
points.InsertNextPoint(-1,0,0)
points.InsertNextPoint(0,0,0)
points.InsertNextPoint(1,0,0)
points.InsertNextPoint(-1,-1,0)
points.InsertNextPoint(0,-1,0)
points.InsertNextPoint(1,-1,0)
faceColors = vtk.vtkFloatArray()
faceColors.InsertNextValue(0)
faceColors.InsertNextValue(1)
faceColors.InsertNextValue(1)
faceColors.InsertNextValue(2)
sgrid = vtk.vtkStructuredGrid()
sgrid.SetDimensions(3,3,1)
sgrid.SetPoints(points)
sgrid.GetCellData().SetScalars(faceColors)
Cell2Point = vtk.vtkCellDataToPointData()
Cell2Point.SetInputData(sgrid)
Cell2Point.PassCellDataOn()
Cell2Point.Update()
mapper = vtk.vtkDataSetMapper()
mapper.SetInputData(Cell2Point.GetStructuredGridOutput())
mapper.SetScalarModeToUsePointData()
mapper.SetScalarRange(0,2)
actor = vtk.vtkActor()
actor.SetMapper(mapper)
# Add the actors to the renderer, set the background and size
ren1.AddActor(actor)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(256,256)
# render the image
iren.Initialize()
# --- end of script --
