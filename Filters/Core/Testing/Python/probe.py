#!/usr/bin/env python


import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()


# Create the RenderWindow, Renderer and both Actors
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# cut data
pl3d = vtk.vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)

plane = vtk.vtkPlane()
plane.SetOrigin(output.GetCenter())
plane.SetNormal(-0.287, 0, 0.9579)

planeCut = vtk.vtkCutter()
planeCut.SetInputData(output)
planeCut.SetCutFunction(plane)

probe = vtk.vtkProbeFilter()
probe.SetInputConnection(planeCut.GetOutputPort())
probe.SetSourceData(output)

cutMapper = vtk.vtkDataSetMapper()
cutMapper.SetInputConnection(probe.GetOutputPort())
cutMapper.SetScalarRange(output.GetPointData().GetScalars().GetRange())

cutActor = vtk.vtkActor()
cutActor.SetMapper(cutMapper)

#extract plane
compPlane = vtk.vtkStructuredGridGeometryFilter()
compPlane.SetInputData(output)
compPlane.SetExtent(0, 100, 0, 100, 9, 9)

planeMapper = vtk.vtkPolyDataMapper()
planeMapper.SetInputConnection(compPlane.GetOutputPort())
planeMapper.ScalarVisibilityOff()

planeActor = vtk.vtkActor()
planeActor.SetMapper(planeMapper)
planeActor.GetProperty().SetRepresentationToWireframe()
planeActor.GetProperty().SetColor(0, 0, 0)

#outline
outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInputData(output)

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

outlineProp = outlineActor.GetProperty()
outlineProp.SetColor(0, 0, 0)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(planeActor)
ren.AddActor(cutActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(400, 300)

cam1 = ren.GetActiveCamera()
cam1.SetClippingRange(11.1034, 59.5328)
cam1.SetFocalPoint(9.71821, 0.458166, 29.3999)
cam1.SetPosition(-2.95748, -26.7271, 44.5309)
cam1.SetViewUp(0.0184785, 0.479657, 0.877262)

iren.Initialize()
renWin.Render()
#iren.Start()
