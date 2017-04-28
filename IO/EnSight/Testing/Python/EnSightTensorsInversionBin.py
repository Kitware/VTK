#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create a rendering window and renderer
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.StereoCapableWindowOn()
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
reader = vtk.vtkGenericEnSightReader()
reader.SetCaseFileName("" + str(VTK_DATA_ROOT) + "/Data/EnSight/elements6-bin.case")
reader.UpdateInformation()
reader.GetOutputInformation(0).Set(vtk.vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP(), 0.1)
geom = vtk.vtkGeometryFilter()
geom.SetInputConnection(reader.GetOutputPort())
calc = vtk.vtkArrayCalculator()
calc.SetInputConnection(geom.GetOutputPort())
calc.SetAttributeModeToUsePointData()
calc.SetFunction("pointTensors_XZ - pointTensors_YZ")
calc.AddScalarVariable("pointTensors_XZ","pointTensors", 5)
calc.AddScalarVariable("pointTensors_YZ","pointTensors", 4)
calc.SetResultArrayName("test")
mapper = vtk.vtkHierarchicalPolyDataMapper()
mapper.SetInputConnection(calc.GetOutputPort())
mapper.SetColorModeToMapScalars()
mapper.SetScalarModeToUsePointFieldData()
mapper.ColorByArrayComponent("test",0)
mapper.SetScalarRange(-0.1,0.1)
actor = vtk.vtkActor()
actor.SetMapper(mapper)
# assign our actor to the renderer
ren1.AddActor(actor)
# enable user interface interactor
iren.Initialize()
renWin.Render()
# prevent the tk window from showing up then start the event loop
reader.SetDefaultExecutivePrototype(None)
# --- end of script --
