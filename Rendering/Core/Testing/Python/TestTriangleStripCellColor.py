#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

sphere = vtk.vtkSphereSource()
elevation = vtk.vtkElevationFilter()
elevation.SetInputConnection(sphere.GetOutputPort())
elevation.SetLowPoint(-1,0,0)
elevation.SetHighPoint(1,0,0)
p2c = vtk.vtkPointDataToCellData()
p2c.SetInputConnection(elevation.GetOutputPort())
stripper = vtk.vtkStripper()
stripper.SetInputConnection(p2c.GetOutputPort())
stripper.PassCellDataAsFieldDataOn()
sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(stripper.GetOutputPort())
sphereMapper.SelectColorArray("Elevation")
sphereMapper.SetColorModeToMapScalars()
sphereMapper.SetScalarModeToUseFieldData()
sphereMapper.SetScalarRange(0.28,0.72)
sphereActor = vtk.vtkActor()
sphereActor.SetMapper(sphereMapper)
#interpolation must be set to flat for cell colors to work
#for triangle strips.
sphereActor.GetProperty().SetInterpolationToFlat()
ren1 = vtk.vtkRenderer()
ren1.AddActor(sphereActor)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(200,200)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
renWin.Render()
# --- end of script --
