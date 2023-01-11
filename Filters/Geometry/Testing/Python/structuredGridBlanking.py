#!/usr/bin/env python

#test exists to verify that structured grid blanking works as expected

from vtkmodules.vtkCommonCore import (
    vtkDoubleArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import vtkStructuredGrid
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

#make up a toy structured grid with known characteristics
xlim=10
ylim=10
zlim=3

sg = vtkStructuredGrid()
sg.SetExtent(0,xlim,0,ylim,0,zlim)

#a handy point iterator, calls action() on each point
def forEachPoint(xlim,ylim,zlim, action):
    for z in range(0,zlim+1):
        for y in range(0,ylim+1):
            for x in range(0,xlim+1):
                 action((x,y,z))

#make geometry
points = vtkPoints()
def makeCoordinate(pt):
    points.InsertNextPoint(pt)
forEachPoint(xlim,ylim,zlim, makeCoordinate)
sg.SetPoints(points)

#make a scalar array
scalars = vtkDoubleArray()
scalars.SetNumberOfComponents(1)
scalars.SetName("Xcoord")
def makeScalar(pt):
    scalars.InsertNextValue(pt[0]+pt[1]+pt[2])
forEachPoint(xlim,ylim,zlim, makeScalar)
sg.GetPointData().SetScalars(scalars)

#blank some arbitrarily chosen cells
numcells = sg.GetNumberOfCells()
if 11 < numcells:
    sg.BlankCell(11)
if 64 < numcells:
    sg.BlankCell(64)
if 164 < numcells:
    sg.BlankCell(164)
for c in range(180,261):
    if c < sg.GetNumberOfCells():
        sg.BlankCell(c)

dsf = vtkDataSetSurfaceFilter()
dsf.SetInputData(sg)
dsf.Update()
nviscells = dsf.GetOutput().GetNumberOfCells()
if nviscells != 356:
    print("Problem")
    print("Test expected 356 visible surface polygons but got", \
          nviscells)
    exit(-1)

#render it so we can look at it
mapper = vtkDataSetMapper()
mapper.SetInputData(sg)
mapper.SetScalarRange(scalars.GetRange())
actor = vtkActor()
actor.SetMapper(mapper)
ren = vtkRenderer()
ren.AddActor(actor)
renWin = vtkRenderWindow()
renWin.SetSize(400, 400)
renWin.AddRenderer(ren)

#set position where we can see most of the blanked cells
cam = ren.GetActiveCamera()
cam.SetClippingRange(14.0456, 45.4716)
cam.SetFocalPoint(5, 5, 1.5)
cam.SetPosition(-19.0905, -6.73006, -6.37738)
cam.SetViewUp(-0.400229, 0.225459, 0.888248)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()

#iren.Start()
