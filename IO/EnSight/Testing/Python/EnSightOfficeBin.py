#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# read data
#
reader = vtk.vtkGenericEnSightReader()
# Make sure all algorithms use the composite data pipeline
cdp = vtk.vtkCompositeDataPipeline()
reader.SetDefaultExecutivePrototype(cdp)
reader.SetCaseFileName("" + str(VTK_DATA_ROOT) + "/Data/EnSight/office_bin.case")
reader.Update()
outline = vtk.vtkStructuredGridOutlineFilter()
#    outline SetInputConnection [reader GetOutputPort]
outline.SetInputData(reader.GetOutput().GetBlock(0))
mapOutline = vtk.vtkPolyDataMapper()
mapOutline.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(mapOutline)
outlineActor.GetProperty().SetColor(0,0,0)
# Create source for streamtubes
streamer = vtk.vtkStreamTracer()
#    streamer SetInputConnection [reader GetOutputPort]
streamer.SetInputData(reader.GetOutput().GetBlock(0))
streamer.SetStartPosition(0.1,2.1,0.5)
streamer.SetMaximumPropagation(500)
streamer.SetInitialIntegrationStep(0.1)
streamer.SetIntegrationDirectionToForward()
cone = vtk.vtkConeSource()
cone.SetResolution(8)
cones = vtk.vtkGlyph3D()
cones.SetInputConnection(streamer.GetOutputPort())
cones.SetSourceConnection(cone.GetOutputPort())
cones.SetScaleFactor(3)
cones.SetInputArrayToProcess(1, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_POINTS, "vectors")
cones.SetScaleModeToScaleByVector()
mapCones = vtk.vtkPolyDataMapper()
mapCones.SetInputConnection(cones.GetOutputPort())
#    eval mapCones SetScalarRange [[reader GetOutput] GetScalarRange]
mapCones.SetScalarRange(reader.GetOutput().GetBlock(0).GetScalarRange())
conesActor = vtk.vtkActor()
conesActor.SetMapper(mapCones)
ren1.AddActor(outlineActor)
ren1.AddActor(conesActor)
ren1.SetBackground(0.4,0.4,0.5)
renWin.SetSize(300,300)
iren.Initialize()
# interact with data
reader.SetDefaultExecutivePrototype(None)
# --- end of script --
