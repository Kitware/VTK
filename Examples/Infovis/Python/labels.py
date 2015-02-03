#!/usr/bin/env python

import random
from vtk import *

n = 10000

qinit = vtkQtInitialization()

pd = vtkPolyData()
pts = vtkPoints()
verts = vtkCellArray()
orient = vtkDoubleArray()
orient.SetName('orientation')
label = vtkStringArray()
label.SetName('label')
for i in range(n):
  pts.InsertNextPoint(random.random(), random.random(), random.random())
  verts.InsertNextCell(1)
  verts.InsertCellPoint(i)
  orient.InsertNextValue(random.random()*360.0)
  label.InsertNextValue(str(i))

pd.SetPoints(pts)
pd.SetVerts(verts)
pd.GetPointData().AddArray(label)
pd.GetPointData().AddArray(orient)

hier = vtkPointSetToLabelHierarchy()
hier.SetInputData(pd)
hier.SetOrientationArrayName('orientation')
hier.SetLabelArrayName('label')
hier.GetTextProperty().SetColor(0.0, 0.0, 0.0)

lmapper = vtkLabelPlacementMapper()
lmapper.SetInputConnection(hier.GetOutputPort())
strategy = vtkQtLabelRenderStrategy()
lmapper.SetRenderStrategy(strategy)
lmapper.SetShapeToRoundedRect()
lmapper.SetBackgroundColor(1.0, 1.0, 0.7)
lmapper.SetBackgroundOpacity(0.8)
lmapper.SetMargin(3)

lactor = vtkActor2D()
lactor.SetMapper(lmapper)

mapper = vtkPolyDataMapper()
mapper.SetInputData(pd)
actor = vtkActor()
actor.SetMapper(mapper)

ren = vtkRenderer()
ren.AddActor(lactor)
ren.AddActor(actor)
ren.ResetCamera()

win = vtkRenderWindow()
win.AddRenderer(ren)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(win)

iren.Initialize()
iren.Start()

