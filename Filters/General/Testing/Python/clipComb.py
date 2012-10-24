#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create pipeline
#
pl3d = vtk.vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName("" + str(VTK_DATA_ROOT) + "/Data/combxyz.bin")
pl3d.SetQFileName("" + str(VTK_DATA_ROOT) + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)
# create a crazy implicit function
center = output.GetCenter()
sphere = vtk.vtkSphere()
sphere.SetCenter(center)
sphere.SetRadius(2.0)
sphere2 = vtk.vtkSphere()
sphere2.SetCenter(expr.expr(globals(), locals(),["lindex(center,0)","+","4.0"]),lindex(center,1),lindex(center,2))
sphere2.SetRadius(4.0)
bool = vtk.vtkImplicitBoolean()
bool.SetOperationTypeToUnion()
bool.AddFunction(sphere)
bool.AddFunction(sphere2)
# clip the structured grid to produce a tetrahedral mesh
clip = vtk.vtkClipDataSet()
clip.SetInputData(output)
clip.SetClipFunction(bool)
clip.InsideOutOn()
gf = vtk.vtkGeometryFilter()
gf.SetInputConnection(clip.GetOutputPort())
clipMapper = vtk.vtkPolyDataMapper()
clipMapper.SetInputConnection(gf.GetOutputPort())
clipActor = vtk.vtkActor()
clipActor.SetMapper(clipMapper)
outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInputData(output)
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(clipActor)
ren1.AddActor(outlineActor)
ren1.SetBackground(1,1,1)
renWin.SetSize(250,250)
ren1.SetBackground(0.1,0.2,0.4)
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(8.88908,0.595038,29.3342)
cam1.SetPosition(-12.3332,31.7479,41.2387)
cam1.SetViewUp(0.060772,-0.319905,0.945498)
iren.Initialize()
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
