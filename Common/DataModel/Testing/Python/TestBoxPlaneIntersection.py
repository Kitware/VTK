#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkBox,
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkFiltersSources import vtkCubeSource
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
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer( ren )
renWin.SetSize(600,200)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a cube and cut it with a plane
#
boxL = vtkCubeSource()
boxL.SetBounds(-2.5,-1.5, -0.5,0.5, -0.5,0.5)

boxC = vtkCubeSource()
boxC.SetBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)

boxR = vtkCubeSource()
boxR.SetBounds(1.5,2.5, -0.5,0.5, -0.5,0.5)

mapperL = vtkPolyDataMapper()
mapperL.SetInputConnection(boxL.GetOutputPort())

mapperC = vtkPolyDataMapper()
mapperC.SetInputConnection(boxC.GetOutputPort())

mapperR = vtkPolyDataMapper()
mapperR.SetInputConnection(boxR.GetOutputPort())

actorL = vtkActor()
actorL.SetMapper(mapperL)
actorL.GetProperty().SetRepresentationToWireframe()
actorL.GetProperty().SetAmbient(1)

actorC = vtkActor()
actorC.SetMapper(mapperC)
actorC.GetProperty().SetRepresentationToWireframe()
actorC.GetProperty().SetAmbient(1)

actorR = vtkActor()
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
clipBox = vtkBox()

# Left
normal = [1,1,1]
origin = boxL.GetCenter()
pdL = vtkPolyData()
polyL = vtkCellArray()
ptsL = vtkPoints()
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
mapperPL = vtkPolyDataMapper()
mapperPL.SetInputData(pdL)
actorPL = vtkActor()
actorPL.SetMapper(mapperPL)
ren.AddActor(actorPL)

# Center
normal = [.4,.8,.4]
origin = boxC.GetCenter()
pdC = vtkPolyData()
polyC = vtkCellArray()
ptsC = vtkPoints()
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
mapperPC = vtkPolyDataMapper()
mapperPC.SetInputData(pdC)
actorPC = vtkActor()
actorPC.SetMapper(mapperPC)
ren.AddActor(actorPC)

# Right
normal = [0,0,1]
origin = boxR.GetCenter()
pdR = vtkPolyData()
polyR = vtkCellArray()
ptsR = vtkPoints()
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
mapperPR = vtkPolyDataMapper()
mapperPR.SetInputData(pdR)
actorPR = vtkActor()
actorPR.SetMapper(mapperPR)
ren.AddActor(actorPR)

ren.GetActiveCamera().SetFocalPoint(0,0,0)
ren.GetActiveCamera().SetPosition(0,0.5,1)
ren.ResetCamera()
ren.GetActiveCamera().Zoom(2.5)

renWin.Render()
iren.Start()
