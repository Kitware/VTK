#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create a rendering window and renderer
ren1 = vtk.vtkRenderer()
ren1.SetBackground(0,0,0)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(300,300)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# camera parameters
camera = ren1.GetActiveCamera()
camera.SetPosition(-54.8012,109.471,231.412)
camera.SetFocalPoint(33,33,33)
camera.SetViewUp(0.157687,0.942832,-0.293604)
camera.SetViewAngle(30)
camera.SetClippingRange(124.221,363.827)
reader = vtk.vtkGenericEnSightReader()
# Make sure all algorithms use the composite data pipeline
cdp = vtk.vtkCompositeDataPipeline()
reader.SetDefaultExecutivePrototype(cdp)
reader.SetCaseFileName("" + str(VTK_DATA_ROOT) + "/Data/EnSight/ironProt_bin.case")
Contour0 = vtk.vtkContourFilter()
Contour0.SetInputConnection(reader.GetOutputPort())
Contour0.SetValue(0,200)
Contour0.SetComputeScalars(1)
mapper = vtk.vtkHierarchicalPolyDataMapper()
mapper.SetInputConnection(Contour0.GetOutputPort())
mapper.SetImmediateModeRendering(1)
mapper.SetScalarRange(0,1)
mapper.SetScalarVisibility(1)
actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetRepresentationToSurface()
actor.GetProperty().SetInterpolationToGouraud()
ren1.AddActor(actor)
# enable user interface interactor
iren.Initialize()
# prevent the tk window from showing up then start the event loop
reader.SetDefaultExecutivePrototype(None)
# --- end of script --
