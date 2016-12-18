#!/usr/bin/env python

import vtk
import math
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create a vtkMultiBlockDataSet with one
# vtkPolyData block, rendered using a vtkCompositePolyDataMapper2
# and picked using vtkPointPicker and vtkCellPicker.

coords=[[0,0,0], [1,0,0], [1,1,0], [0,1,0], [0,0,1], [1,0,1], [1,1,1], [0,1,1]]
pts=vtk.vtkPoints()
scalar=vtk.vtkFloatArray()
v=1
for coord in coords:
    pts.InsertNextPoint(coord[0],coord[1],coord[2])
    scalar.InsertNextValue(v)
    v+=v

cells=[[3,2,1,0],[0,1,5,4],[1,2,6,5],[2,3,7,6],[3,0,4,7],[4,5,6,7]]
polys=vtk.vtkCellArray()
for cell in cells:
    polys.InsertNextCell(4)
    for pid in cell:
        polys.InsertCellPoint(pid)

poly=vtk.vtkPolyData()
poly.SetPoints(pts)
poly.SetPolys(polys)
poly.GetPointData().SetScalars(scalar)

mbd=vtk.vtkMultiBlockDataSet()
mbd.SetNumberOfBlocks(1)
mbd.SetBlock(0,poly)

m=vtk.vtkCompositePolyDataMapper2()
m.SetInputDataObject(mbd)
m.SetScalarModeToUsePointData()

# Override scalar representation with solid color
a=vtk.vtkActor()
a.SetMapper(m)
m.ScalarVisibilityOff()
a.GetProperty().SetColor(1,0,1)

r = vtk.vtkRenderer()
r.AddViewProp(a)
r.SetBackground(0,0,0)
rw = vtk.vtkRenderWindow()
rw.AddRenderer(r)
rw.Render()

#rwi = vtk.vtkRenderWindowInteractor()
#rwi.SetRenderWindow(rw)
#rwi.Start()

cellPicker = vtk.vtkCellPicker()
pointPicker = vtk.vtkPointPicker()
worldPicker = vtk.vtkWorldPointPicker()
cellCount = 0
pointCount = 0
r.IsInViewport(0,0)
p=list(rw.GetSize())
p[0]=p[0]/2
p[1]=p[1]/2
n=36
for i in range(0,n):
    r.GetActiveCamera().Azimuth(360/n)

    pos=list(r.GetActiveCamera().GetPosition())
    rw.Render()
    worldPicker.Pick(p[0],p[1],0,r)
    cid=cellPicker.Pick(p[0],p[1],0,r)
    if cellPicker.GetDataSet():
        print('camera at {} pick at {} picked cell {}'.format(pos,p,cellPicker.GetCellId()))
    pid=pointPicker.Pick(p[0],p[1],0,r)
    if pointPicker.GetDataSet():
        print('camera at {} pick at {} picked point {}'.format(pos,p,pointPicker.GetPointId()))

# render the image
# --- end of script --
