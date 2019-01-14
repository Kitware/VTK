#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren )
renWin.SetSize(600,200)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a cube and cut it with a plane
#
boxL = vtk.vtkCubeSource()
boxL.SetBounds(-2.5,-1.5, -0.5,0.5, -0.5,0.5)

boxC = vtk.vtkCubeSource()
boxC.SetBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)

boxR = vtk.vtkCubeSource()
boxR.SetBounds(1.5,2.5, -0.5,0.5, -0.5,0.5)

mapperL = vtk.vtkPolyDataMapper()
mapperL.SetInputConnection(boxL.GetOutputPort())

mapperC = vtk.vtkPolyDataMapper()
mapperC.SetInputConnection(boxC.GetOutputPort())

mapperR = vtk.vtkPolyDataMapper()
mapperR.SetInputConnection(boxR.GetOutputPort())

actorL = vtk.vtkActor()
actorL.SetMapper(mapperL)
actorL.GetProperty().SetRepresentationToWireframe()
actorL.GetProperty().SetAmbient(1)

actorC = vtk.vtkActor()
actorC.SetMapper(mapperC)
actorC.GetProperty().SetRepresentationToWireframe()
actorC.GetProperty().SetAmbient(1)

actorR = vtk.vtkActor()
actorR.SetMapper(mapperR)
actorR.GetProperty().SetRepresentationToWireframe()
actorR.GetProperty().SetAmbient(1)

ren.AddActor(actorL)
ren.AddActor(actorC)
ren.AddActor(actorR)

# Now clip boxes
origin = [0,0,0]
xout = [0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0]
bds = [0,0, 0,0, 0,0]
clipBox = vtk.vtkBox()

# Left
normal = [1,1,1]
origin = boxL.GetCenter()
pdL = vtk.vtkPolyData()
polyL = vtk.vtkCellArray()
ptsL = vtk.vtkPoints()
pdL.SetPoints(ptsL)
pdL.SetPolys(polyL)
ptsL.SetDataTypeToDouble()
boxL.GetBounds(bds)
numInts = clipBox.IntersectWithPlane(bds, origin, normal, xout)
print("Num ints: ", numInts)
ptsL.SetNumberOfPoints(numInts)
polyL.InsertNextCell(numInts)
for i in range(0,numInts):
    ptsL.SetPoint(i,xout[3*i],xout[3*i+1],xout[3*i+2])
    polyL.InsertCellPoint(i)
mapperPL = vtk.vtkPolyDataMapper()
mapperPL.SetInputData(pdL)
actorPL = vtk.vtkActor()
actorPL.SetMapper(mapperPL)
ren.AddActor(actorPL)

# Center
normal = [.4,.8,.4]
origin = boxC.GetCenter()
pdC = vtk.vtkPolyData()
polyC = vtk.vtkCellArray()
ptsC = vtk.vtkPoints()
pdC.SetPoints(ptsC)
pdC.SetPolys(polyC)
ptsC.SetDataTypeToDouble()
boxC.GetBounds(bds)
numInts = clipBox.IntersectWithPlane(bds, origin, normal, xout)
print("Num ints: ", numInts)
ptsC.SetNumberOfPoints(numInts)
polyC.InsertNextCell(numInts)
for i in range(0,numInts):
    ptsC.SetPoint(i,xout[3*i],xout[3*i+1],xout[3*i+2])
    polyC.InsertCellPoint(i)
mapperPC = vtk.vtkPolyDataMapper()
mapperPC.SetInputData(pdC)
actorPC = vtk.vtkActor()
actorPC.SetMapper(mapperPC)
ren.AddActor(actorPC)

# Right
normal = [0,0,1]
origin = boxR.GetCenter()
pdR = vtk.vtkPolyData()
polyR = vtk.vtkCellArray()
ptsR = vtk.vtkPoints()
pdR.SetPoints(ptsR)
pdR.SetPolys(polyR)
ptsR.SetDataTypeToDouble()
boxR.GetBounds(bds)
numInts = clipBox.IntersectWithPlane(bds, origin, normal, xout)
print("Num ints: ", numInts)
ptsR.SetNumberOfPoints(numInts)
polyR.InsertNextCell(numInts)
for i in range(0,numInts):
    ptsR.SetPoint(i,xout[3*i],xout[3*i+1],xout[3*i+2])
    polyR.InsertCellPoint(i)
mapperPR = vtk.vtkPolyDataMapper()
mapperPR.SetInputData(pdR)
actorPR = vtk.vtkActor()
actorPR.SetMapper(mapperPR)
ren.AddActor(actorPR)

ren.GetActiveCamera().SetFocalPoint(0,0,0)
ren.GetActiveCamera().SetPosition(0,0.5,1)
ren.ResetCamera()
ren.GetActiveCamera().Zoom(2.5)

renWin.Render()
iren.Start()
