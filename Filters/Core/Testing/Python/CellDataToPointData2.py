#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test the fast paths in vtkCellDataToPointData
# Create three side-by-side windows to compare
# the results on polydata and unstructured grid.

# Control test resolution
res = 20

# Create the RenderWindow, Renderer and RenderWindowInteractor
ren0 = vtk.vtkRenderer()
ren0.SetViewport(0,0,0.33,1.0)
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0.33,0,0.66,1.0)
ren2 = vtk.vtkRenderer()
ren2.SetViewport(0.66,0,1.0,1.0)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a sphere with cell data
sphere = vtk.vtkSphereSource()
sphere.SetThetaResolution(res)
sphere.SetPhiResolution(int(res/2))
sphere.GenerateNormalsOff()

ele = vtk.vtkSimpleElevationFilter()
ele.SetInputConnection(sphere.GetOutputPort())

pd2cd = vtk.vtkPointDataToCellData()
pd2cd.SetInputConnection(ele.GetOutputPort())
pd2cd.PassPointDataOff()

m0 = vtk.vtkPolyDataMapper()
m0.SetInputConnection(pd2cd.GetOutputPort())

a0 = vtk.vtkActor()
a0.SetMapper(m0)


# Now test on polydata
cd2pd1 = vtk.vtkCellDataToPointData()
cd2pd1.SetInputConnection(pd2cd.GetOutputPort())

m1 = vtk.vtkPolyDataMapper()
m1.SetInputConnection(cd2pd1.GetOutputPort())

a1 = vtk.vtkActor()
a1.SetMapper(m1)

# Now test on unstructured grid
impFunc = vtk.vtkSphere()
impFunc.SetCenter(0,0,0)
impFunc.SetRadius(10000000)
extract = vtk.vtkExtractGeometry()
extract.SetImplicitFunction(impFunc)
extract.SetInputConnection(pd2cd.GetOutputPort())
extract.ExtractInsideOn()
extract.Update()

cd2pd2 = vtk.vtkCellDataToPointData()
cd2pd2.SetInputConnection(extract.GetOutputPort())

m2 = vtk.vtkDataSetMapper()
m2.SetInputConnection(cd2pd2.GetOutputPort())

a2 = vtk.vtkActor()
a2.SetMapper(m2)

# Add the actors to the renderer, set the background and size
ren0.AddActor(a0)
ren0.SetBackground(0.1,0.2,0.4)
ren1.AddActor(a1)
ren1.SetBackground(0.1,0.2,0.4)
ren2.AddActor(a2)
ren2.SetBackground(0.1,0.2,0.4)
renWin.SetSize(600,200)

ren0.GetActiveCamera().SetPosition(1,0,0)
ren0.GetActiveCamera().SetFocalPoint(0,0,0)
ren0.ResetCamera()
ren1.SetActiveCamera(ren0.GetActiveCamera())
ren2.SetActiveCamera(ren0.GetActiveCamera())

# render the image
iren.Initialize()
iren.Start()
# --- end of script --
