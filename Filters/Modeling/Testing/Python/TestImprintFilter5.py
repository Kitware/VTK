#!/usr/bin/env python
import vtk

# Control the resolution of the test
radius = 10.0
res = 4

# Create pipeline. Use two sphere sources:
# a portion of one sphere imprints on the other.
#
target = vtk.vtkSphereSource()
target.SetRadius(radius)
target.SetCenter(0,0,0)
target.LatLongTessellationOn()
target.SetThetaResolution(4*res)
target.SetPhiResolution(4*res)
target.SetStartTheta(0)
target.SetEndTheta(90)

imprint = vtk.vtkSphereSource()
imprint.SetRadius(radius)
imprint.SetCenter(0,0,0)
imprint.LatLongTessellationOn()
imprint.SetThetaResolution(8*res)
imprint.SetPhiResolution(4*res)
imprint.SetStartTheta(12)
imprint.SetEndTheta(57)
imprint.SetStartPhi(60.0)
imprint.SetEndPhi(120.0)

# Produce an imprint
imp = vtk.vtkImprintFilter()
imp.SetTargetConnection(target.GetOutputPort())
imp.SetImprintConnection(imprint.GetOutputPort())
imp.SetOutputTypeToProjectedImprint()
imp.SetTolerance(0.085)
imp.Update()

impMapper = vtk.vtkPolyDataMapper()
impMapper.SetInputConnection(imp.GetOutputPort())
impMapper.SetScalarRange(0,2)
impMapper.SetScalarModeToUseCellFieldData()
impMapper.SelectColorArray("ImprintedCells")

impActor = vtk.vtkActor()
impActor.SetMapper(impMapper)
impActor.GetProperty().SetColor(1,0,0)

imprintMapper = vtk.vtkPolyDataMapper()
imprintMapper.SetInputConnection(imprint.GetOutputPort())

imprintActor = vtk.vtkActor()
imprintActor.SetMapper(imprintMapper)
imprintActor.GetProperty().SetColor(1,1,1)

# Create the RenderWindow, Renderer
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren1 )
renWin.SetSize(300,200)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(impActor)
#ren1.AddActor(imprintActor)

cam = ren1.GetActiveCamera()
cam.SetPosition(1,0.5,0)
cam.SetFocalPoint(0,0,0)
ren1.ResetCamera()

renWin.Render()
iren.Start()
