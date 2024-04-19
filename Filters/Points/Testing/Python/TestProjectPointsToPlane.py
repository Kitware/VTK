#!/usr/bin/env python
# -*- coding: utf-8 -*-

from vtkmodules.vtkFiltersPoints import vtkProjectPointsToPlane
from vtkmodules.vtkFiltersSources import vtkDiskSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing

# Control test resolution
res = 10

# Controls the plane normal.
normal = [.8,.9,1]

# Create a disk and project it in a variety of ways.
disk = vtkDiskSource()
disk.SetInnerRadius(1)
disk.SetOuterRadius(9)
disk.SetRadialResolution(res)
disk.SetCircumferentialResolution(4*res)
disk.SetCenter(10,20,30)
disk.SetNormal(normal)
disk.Update()
print("Disk Center: ", disk.GetOutput().GetCenter())
print("Disk Normal: ", normal)

diskMapper = vtkPolyDataMapper()
diskMapper.SetInputConnection(disk.GetOutputPort())

diskActor = vtkActor()
diskActor.SetMapper(diskMapper)
diskActor.GetProperty().SetColor(0.85,0.5,0.5)

# Project to the x plane.
dProj0 = vtkProjectPointsToPlane()
dProj0.SetInputConnection(disk.GetOutputPort())
dProj0.SetProjectionTypeToXPlane()

dMapper0 = vtkPolyDataMapper()
dMapper0.SetInputConnection(dProj0.GetOutputPort())

gActor0 = vtkActor()
gActor0.SetMapper(dMapper0)
gActor0.GetProperty().SetRepresentationToWireframe()
gActor0.GetProperty().SetColor(1,1,1)

# Project to the y plane
dProj1 = vtkProjectPointsToPlane()
dProj1.SetInputConnection(disk.GetOutputPort())
dProj1.SetProjectionTypeToYPlane()

dMapper1 = vtkPolyDataMapper()
dMapper1.SetInputConnection(dProj1.GetOutputPort())

gActor1 = vtkActor()
gActor1.SetMapper(dMapper1)
gActor1.GetProperty().SetColor(1,1,1)
gActor1.GetProperty().SetRepresentationToWireframe()

# Project to the z plane
dProj2 = vtkProjectPointsToPlane()
dProj2.SetInputConnection(disk.GetOutputPort())
dProj2.SetProjectionTypeToZPlane()

dMapper2 = vtkPolyDataMapper()
dMapper2.SetInputConnection(dProj2.GetOutputPort())

gActor2 = vtkActor()
gActor2.SetMapper(dMapper2)
gActor2.GetProperty().SetColor(1,1,1)
gActor2.GetProperty().SetRepresentationToWireframe()

# Project to the best fitting planea
dProj3 = vtkProjectPointsToPlane()
dProj3.SetInputConnection(disk.GetOutputPort())
dProj3.SetProjectionTypeToBestFitPlane()
dProj3.Update()
print("Origin: ", dProj3.GetOrigin())
print("Normal: ", dProj3.GetNormal())

dMapper3 = vtkPolyDataMapper()
dMapper3.SetInputConnection(dProj3.GetOutputPort())

gActor3 = vtkActor()
gActor3.SetMapper(dMapper3)
gActor3.GetProperty().SetColor(1,1,1)
gActor3.GetProperty().SetRepresentationToWireframe()

# Project to a specified plane
specNormal = [.1,.2,.4]
dProj4 = vtkProjectPointsToPlane()
dProj4.SetInputConnection(disk.GetOutputPort())
dProj4.SetProjectionTypeToSpecifiedPlane()
dProj4.SetOrigin(1,1,1)
dProj4.SetNormal(specNormal)

dMapper4 = vtkPolyDataMapper()
dMapper4.SetInputConnection(dProj4.GetOutputPort())

gActor4 = vtkActor()
gActor4.SetMapper(dMapper4)
gActor4.GetProperty().SetColor(1,1,1)
gActor4.GetProperty().SetRepresentationToWireframe()

# Project to the best fit coordinate plane
dProj5 = vtkProjectPointsToPlane()
dProj5.SetInputConnection(disk.GetOutputPort())
dProj5.SetProjectionTypeToBestCoordinatePlane()

dMapper5 = vtkPolyDataMapper()
dMapper5.SetInputConnection(dProj5.GetOutputPort())

gActor5 = vtkActor()
gActor5.SetMapper(dMapper5)
gActor5.GetProperty().SetColor(1,1,1)
gActor5.GetProperty().SetRepresentationToWireframe()

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
ren0.SetViewport(0, 0, 0.333, 0.5)
ren1 = vtkRenderer()
ren1.SetViewport(0.333, 0, 0.667, 0.5)
ren2 = vtkRenderer()
ren2.SetViewport(0.667, 0, 1, 0.5)
ren3 = vtkRenderer()
ren3.SetViewport(0, 0.5, 0.333, 1)
ren4 = vtkRenderer()
ren4.SetViewport(0.333, 0.5, 0.667, 1)
ren5 = vtkRenderer()
ren5.SetViewport(0.667, 0.5, 1, 1)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)
renWin.AddRenderer(ren5)

iRen = vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
#ren0.AddActor(diskActor)
ren0.AddActor(gActor0)
ren0.SetBackground(0,0,0)
ren0.GetActiveCamera().SetFocalPoint(0,0,0)
ren0.GetActiveCamera().SetPosition(1,0,0)

#ren1.AddActor(diskActor)
ren1.AddActor(gActor1)
ren1.SetBackground(0,0,0)
ren1.GetActiveCamera().SetFocalPoint(0,0,0)
ren1.GetActiveCamera().SetPosition(0,1,0)
ren1.GetActiveCamera().SetViewUp(0,0,1)

#ren2.AddActor(diskActor)
ren2.AddActor(gActor2)
ren2.SetBackground(0,0,0)
ren2.GetActiveCamera().SetFocalPoint(0,0,0)
ren2.GetActiveCamera().SetPosition(0,0,1)

#ren3.AddActor(diskActor)
ren3.AddActor(gActor3)
ren3.SetBackground(0,0,0)
ren3.GetActiveCamera().SetFocalPoint(0,0,0)
ren3.GetActiveCamera().SetPosition(normal)

#ren4.AddActor(diskActor)
ren4.AddActor(gActor4)
ren4.SetBackground(0,0,0)
ren4.GetActiveCamera().SetFocalPoint(0,0,0)
ren4.GetActiveCamera().SetPosition(specNormal)

#ren5.AddActor(diskActor)
ren5.AddActor(gActor5)
ren5.SetBackground(0,0,0)
ren5.GetActiveCamera().SetFocalPoint(0,0,0)
ren5.GetActiveCamera().SetPosition(0,0,1)

renWin.SetSize(600, 400)

iRen.Initialize()
ren0.ResetCamera()
ren1.ResetCamera()
ren2.ResetCamera()
ren3.ResetCamera()
ren4.ResetCamera()
ren5.ResetCamera()
renWin.Render()

# Interact with the data
iRen.Start()
