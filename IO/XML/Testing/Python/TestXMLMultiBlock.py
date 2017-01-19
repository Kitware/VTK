#!/usr/bin/env python

import os
import vtk
from vtk.util.misc import vtkGetDataRoot
from vtk.util.misc import vtkGetTempDir

VTK_DATA_ROOT = vtkGetDataRoot()
VTK_TEMP_DIR = vtkGetTempDir()

w = vtk.vtkRTAnalyticSource()

g = vtk.vtkMultiBlockDataGroupFilter()
g.AddInputConnection(w.GetOutputPort())

g.Update()

mb = g.GetOutputDataObject(0)

a = vtk.vtkFloatArray()
a.SetName("foo")
a.SetNumberOfTuples(1)
a.SetValue(0, 10)

mb.GetFieldData().AddArray(a)

file_root = VTK_TEMP_DIR + '/testxml'
file0 = file_root + ".vtm"

wri = vtk.vtkXMLMultiBlockDataWriter()
wri.SetInputData(mb)
wri.SetFileName(file0)
wri.Write()

read = vtk.vtkXMLMultiBlockDataReader()
read.SetFileName(file0)
read.Update()

output = read.GetOutputDataObject(0)
assert(output.GetFieldData().GetArray("foo"))
assert(output.GetFieldData().GetArray("foo").GetValue(0) == 10)

os.remove(file0)
os.remove(file_root + "/testxml_0.vti")
os.rmdir(file_root)
