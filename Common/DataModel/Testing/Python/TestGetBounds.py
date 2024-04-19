#!/usr/bin/env python
# -*- coding: utf-8 -*-

from vtkmodules.vtkCommonDataModel import vtkBoundingBox
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersCore import (
    vtkAppendPolyData,
    vtkFeatureEdges,
)
from vtkmodules.vtkFiltersGeneral import vtkTransformPolyDataFilter
from vtkmodules.vtkFiltersSources import vtkPlaneSource
import sys

# Test speed of compute bounds in vtkPolyData, vtkPoints, and
# vtkBoundingBox.

# Control model size
res = 500
timer = vtkTimerLog()

# Uncomment if you want to use as a little interactive program
#if len(sys.argv) >= 2 :
#    res = int(sys.argv[1])
#else:
#    res = 500

# Data source. Note that different types of cells are created
# to exercise the vtkPolyData::GetBounds() properly.
plane = vtkPlaneSource()
plane.SetResolution(res,res)

edges = vtkFeatureEdges()
edges.SetInputConnection(plane.GetOutputPort())
#edges.ExtractAllEdgeTypesOff()
edges.BoundaryEdgesOn()
edges.ManifoldEdgesOff()
edges.NonManifoldEdgesOff()
edges.FeatureEdgesOff()

t1 = vtkTransform()
t1.Translate(-1.0,0,0)
tf1 = vtkTransformPolyDataFilter()
tf1.SetInputConnection(edges.GetOutputPort())
tf1.SetTransform(t1)

t2 = vtkTransform()
t2.Translate(1.0,0,0)
tf2 = vtkTransformPolyDataFilter()
tf2.SetInputConnection(edges.GetOutputPort())
tf2.SetTransform(t2)

append = vtkAppendPolyData()
append.AddInputConnection(tf1.GetOutputPort())
append.AddInputConnection(plane.GetOutputPort())
append.AddInputConnection(tf2.GetOutputPort())
append.Update()

output = append.GetOutput()
points = output.GetPoints()
box = [0.0,0.0,0.0,0.0,0.0,0.0]

print("Input data:")
print("\tNum Points: {0}".format(output.GetNumberOfPoints()))
print("\tNum Cells: {0}".format(output.GetNumberOfCells()))

# Compute bounds on polydata
points.Modified()
timer.StartTimer()
output.GetBounds(box)
timer.StopTimer()
time = timer.GetElapsedTime()
print("vtkPolyData::ComputeBounds():")
print("\tTime: {0}".format(time))
print("\tBounds: {0}".format(box))

assert box[0] == -1.5
assert box[1] ==  1.5
assert box[2] == -0.5
assert box[3] ==  0.5
assert box[4] ==  0.0
assert box[5] ==  0.0

# Uses vtkPoints::ComputeBounds() which uses threaded vtkSMPTools and
# vtkArrayDispatch (see vtkDataArrayPrivate.txx). In other words, cell
# connectivity is not taken into account.
points.Modified()
timer.StartTimer()
points.GetBounds(box)
timer.StopTimer()
time = timer.GetElapsedTime()
print("vtkPoints::ComputeBounds():")
print("\tTime: {0}".format(time))
print("\tBounds: {0}".format(box))

assert box[0] == -1.5
assert box[1] ==  1.5
assert box[2] == -0.5
assert box[3] ==  0.5
assert box[4] ==  0.0
assert box[5] ==  0.0

# Uses vtkBoundingBox with vtkSMPTools. This method takes into account
# an (optional) pointUses array to only consider selected points.
bbox = vtkBoundingBox()
timer.StartTimer()
bbox.ComputeBounds(points,box)
timer.StopTimer()
time = timer.GetElapsedTime()
print("vtkBoundingBox::ComputeBounds():")
print("\tTime: {0}".format(time))
print("\tBounds: {0}".format(box))

assert box[0] == -1.5
assert box[1] ==  1.5
assert box[2] == -0.5
assert box[3] ==  0.5
assert box[4] ==  0.0
assert box[5] ==  0.0
