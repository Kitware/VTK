#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtk.vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# Generate implicit model of a sphere
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# pipeline stuff
#
sphere = vtk.vtkSphereSource()
sphere.SetPhiResolution(150)
sphere.SetThetaResolution(150)

pts = vtk.vtkPoints()
pts.InsertNextPoint(0, 0, 0)
pts.InsertNextPoint(1, 0, 0)
pts.InsertNextPoint(0, 1, 0)
pts.InsertNextPoint(0, 0, 1)

tris = vtk.vtkCellArray()
tris.InsertNextCell(3)
tris.InsertCellPoint(0)
tris.InsertCellPoint(1)
tris.InsertCellPoint(2)
tris.InsertNextCell(3)
tris.InsertCellPoint(0)
tris.InsertCellPoint(2)
tris.InsertCellPoint(3)
tris.InsertNextCell(3)
tris.InsertCellPoint(0)
tris.InsertCellPoint(3)
tris.InsertCellPoint(1)
tris.InsertNextCell(3)
tris.InsertCellPoint(1)
tris.InsertCellPoint(2)
tris.InsertCellPoint(3)

polys = vtk.vtkPolyData()
polys.SetPoints(pts)
polys.SetPolys(tris)

mesh = vtk.vtkQuadricClustering()
mesh.SetInputConnection(sphere.GetOutputPort())
mesh.SetNumberOfXDivisions(10)
mesh.SetNumberOfYDivisions(10)
mesh.SetNumberOfZDivisions(10)

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(mesh.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))
actor.GetProperty().SetDiffuse(.8)
actor.GetProperty().SetSpecular(.4)
actor.GetProperty().SetSpecularPower(30)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(300, 300)

iren.Initialize()
#iren.Start()
