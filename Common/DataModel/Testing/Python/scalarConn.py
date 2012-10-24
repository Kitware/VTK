#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Quadric definition
quadric = vtk.vtkQuadric()
quadric.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sample = vtk.vtkSampleFunction()
sample.SetSampleDimensions(30,30,30)
sample.SetImplicitFunction(quadric)
sample.Update()
#sample Print
sample.ComputeNormalsOff()
# Extract cells that contains isosurface of interest
conn = vtk.vtkConnectivityFilter()
conn.SetInputConnection(sample.GetOutputPort())
conn.ScalarConnectivityOn()
conn.SetScalarRange(0.6,0.6)
conn.SetExtractionModeToCellSeededRegions()
conn.AddSeed(105)
# Create a surface
contours = vtk.vtkContourFilter()
contours.SetInputConnection(conn.GetOutputPort())
#  contours SetInputConnection [sample GetOutputPort]
contours.GenerateValues(5,0.0,1.2)
contMapper = vtk.vtkDataSetMapper()
#  contMapper SetInputConnection [contours GetOutputPort]
contMapper.SetInputConnection(conn.GetOutputPort())
contMapper.SetScalarRange(0.0,1.2)
contActor = vtk.vtkActor()
contActor.SetMapper(contMapper)
# Create outline
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
# Graphics
# create a window to render into
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
# create a renderer
# interactiver renderer catches mouse events (optional)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.SetBackground(1,1,1)
ren1.AddActor(contActor)
ren1.AddActor(outlineActor)
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(1.4)
iren.Initialize()
# --- end of script --
