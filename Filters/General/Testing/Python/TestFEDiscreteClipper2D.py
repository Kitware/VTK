#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
ren1.SetBackground(1,1,1)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create synthetic image data
VTK_SHORT = 4
img = vtk.vtkImageData()
img.SetDimensions(6,5,1)
img.AllocateScalars(VTK_SHORT,1)

scalars = img.GetPointData().GetScalars()
scalars.SetTuple1(0,0)
scalars.SetTuple1(1,0)
scalars.SetTuple1(2,0)
scalars.SetTuple1(3,0)
scalars.SetTuple1(4,0)
scalars.SetTuple1(5,0)

scalars.SetTuple1(6,0)
scalars.SetTuple1(7,0)
scalars.SetTuple1(8,0)
scalars.SetTuple1(9,0)
scalars.SetTuple1(10,0)
scalars.SetTuple1(11,0)

scalars.SetTuple1(12,0)
scalars.SetTuple1(13,0)
scalars.SetTuple1(14,0)
scalars.SetTuple1(15,2)
scalars.SetTuple1(16,4)
scalars.SetTuple1(17,0)

scalars.SetTuple1(18,0)
scalars.SetTuple1(19,0)
scalars.SetTuple1(20,1)
scalars.SetTuple1(21,1)
scalars.SetTuple1(22,3)
scalars.SetTuple1(23,3)

scalars.SetTuple1(24,0)
scalars.SetTuple1(25,0)
scalars.SetTuple1(26,3)
scalars.SetTuple1(27,0)
scalars.SetTuple1(28,0)
scalars.SetTuple1(29,3)

# Create the pipeline, extract some regions
discrete = vtk.vtkDiscreteFlyingEdgesClipper2D()
discrete.SetInputData(img)
discrete.SetValue(0,1)
discrete.SetValue(1,2)
discrete.SetValue(2,3)
discrete.SetValue(3,4)
discrete.Update()

# Clipped polygons are generated
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(discrete.GetOutputPort())
mapper.SetScalarModeToUseCellData()
mapper.SetScalarRange(1,4);

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# The image gridlines
gridMapper = vtk.vtkDataSetMapper()
gridMapper.SetInputData(img)
gridMapper.ScalarVisibilityOff()

gridActor = vtk.vtkActor()
gridActor.SetMapper(gridMapper)
gridActor.GetProperty().SetRepresentationToWireframe()
gridActor.GetProperty().SetColor(0,0,1)

ren1.AddActor(actor)
ren1.AddActor(gridActor)

renWin.Render()
iren.Start()
