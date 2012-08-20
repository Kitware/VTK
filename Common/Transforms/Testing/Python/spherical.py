#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# avoid the singularity at the Z axis using 0.0001 radian offset
plane = vtk.vtkPlaneSource()
plane.SetOrigin(1.0,expr.expr(globals(), locals(),["3.14159265359","-","0.0001"]),0.0)
plane.SetPoint1(1.0,expr.expr(globals(), locals(),["3.14159265359","-","0.0001"]),6.28318530719)
plane.SetPoint2(1.0,0.0001,0.0)
plane.SetXResolution(19)
plane.SetYResolution(9)
transform = vtk.vtkSphericalTransform()
tpoly = vtk.vtkTransformPolyDataFilter()
tpoly.SetInputConnection(plane.GetOutputPort())
tpoly.SetTransform(transform)
# also cover the inverse transformation by going back and forth
tpoly2 = vtk.vtkTransformPolyDataFilter()
tpoly2.SetInputConnection(tpoly.GetOutputPort())
tpoly2.SetTransform(transform.GetInverse())
tpoly3 = vtk.vtkTransformPolyDataFilter()
tpoly3.SetInputConnection(tpoly2.GetOutputPort())
tpoly3.SetTransform(transform)
mapper = vtk.vtkDataSetMapper()
mapper.SetInputConnection(tpoly3.GetOutputPort())
earth = vtk.vtkPNMReader()
earth.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/earth.ppm")
texture = vtk.vtkTexture()
texture.SetInputConnection(earth.GetOutputPort())
texture.InterpolateOn()
world = vtk.vtkActor()
world.SetMapper(mapper)
world.SetTexture(texture)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(world)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)
ren1.GetActiveCamera().SetPosition(8,-10,6)
ren1.GetActiveCamera().SetFocalPoint(0,0,0)
ren1.GetActiveCamera().SetViewAngle(15)
ren1.GetActiveCamera().SetViewUp(0.0,0.0,1.0)
# render the image
#
cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.4)
ren1.ResetCameraClippingRange()
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
