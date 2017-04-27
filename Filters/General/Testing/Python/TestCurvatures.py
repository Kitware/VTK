#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create renderer stuff
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Pipeline stuff
#
torus = vtk.vtkSuperquadricSource()
torus.SetCenter(0.0,0.0,0.0)
torus.SetScale(1.0,1.0,1.0)
torus.SetPhiResolution(64)
torus.SetThetaResolution(64)
torus.SetPhiRoundness(1.0)
torus.SetThetaRoundness(1.0)
torus.SetThickness(0.5)
torus.SetSize(0.5)
torus.SetToroidal(1)
# The quadric is made of strips, so pass it through a triangle filter as
# the curvature filter only operates on polys
tri = vtk.vtkTriangleFilter()
tri.SetInputConnection(torus.GetOutputPort())
# The quadric has nasty discontinuities from the way the edges are generated
# so let's pass it though a CleanPolyDataFilter and merge any points which
# are coincident, or very close
cleaner = vtk.vtkCleanPolyData()
cleaner.SetInputConnection(tri.GetOutputPort())
cleaner.SetTolerance(0.005)
curve1 = vtk.vtkCurvatures()
curve1.SetInputConnection(cleaner.GetOutputPort())
curve1.SetCurvatureTypeToGaussian()
curve2 = vtk.vtkCurvatures()
curve2.SetInputConnection(cleaner.GetOutputPort())
curve2.SetCurvatureTypeToMean()
lut1 = vtk.vtkLookupTable()
lut1.SetNumberOfColors(256)
lut1.SetHueRange(0.15,1.0)
lut1.SetSaturationRange(1.0,1.0)
lut1.SetValueRange(1.0,1.0)
lut1.SetAlphaRange(1.0,1.0)
lut1.SetRange(-20,20)
lut2 = vtk.vtkLookupTable()
lut2.SetNumberOfColors(256)
lut2.SetHueRange(0.15,1.0)
lut2.SetSaturationRange(1.0,1.0)
lut2.SetValueRange(1.0,1.0)
lut2.SetAlphaRange(1.0,1.0)
lut2.SetRange(0,4)
cmapper1 = vtk.vtkPolyDataMapper()
cmapper1.SetInputConnection(curve1.GetOutputPort())
cmapper1.SetLookupTable(lut1)
cmapper1.SetUseLookupTableScalarRange(1)
cmapper2 = vtk.vtkPolyDataMapper()
cmapper2.SetInputConnection(curve2.GetOutputPort())
cmapper2.SetLookupTable(lut2)
cmapper2.SetUseLookupTableScalarRange(1)
cActor1 = vtk.vtkActor()
cActor1.SetMapper(cmapper1)
cActor1.SetPosition(-0.5,0.0,0.0)
cActor2 = vtk.vtkActor()
cActor2.SetMapper(cmapper2)
cActor2.SetPosition(0.5,0.0,0.0)
# Add the actors to the renderer
#
ren1.AddActor(cActor1)
ren1.AddActor(cActor2)
ren1.SetBackground(0.5,0.5,0.5)
renWin.SetSize(300,200)
camera = vtk.vtkCamera()
ren1.SetActiveCamera(camera)
camera.SetPosition(0.0,2.0,2.1)
camera.SetFocalPoint(0.0,0.0,0.0)
camera.SetViewAngle(30)
ren1.ResetCameraClippingRange()
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
renWin.Render()
# --- end of script --
