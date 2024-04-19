#!/usr/bin/env python

import os
from vtkmodules.vtkCommonCore import vtkFloatArray
from vtkmodules.vtkFiltersGeneral import vtkMultiBlockDataGroupFilter
from vtkmodules.vtkIOXML import (
    vtkXMLMultiBlockDataReader,
    vtkXMLMultiBlockDataWriter,
)
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.util.misc import vtkGetDataRoot
from vtkmodules.util.misc import vtkGetTempDir

VTK_DATA_ROOT = vtkGetDataRoot()
VTK_TEMP_DIR = vtkGetTempDir()

w = vtkRTAnalyticSource()

g = vtkMultiBlockDataGroupFilter()
g.AddInputConnection(w.GetOutputPort())

g.Update()

mb = g.GetOutputDataObject(0)

a = vtkFloatArray()
a.SetName("foo")
a.SetNumberOfTuples(1)
a.SetValue(0, 10)

mb.GetFieldData().AddArray(a)

file_root = VTK_TEMP_DIR + '/testxml'
file0 = file_root + ".vtm"

wri = vtkXMLMultiBlockDataWriter()
wri.SetInputData(mb)
wri.SetFileName(file0)
wri.Write()

read = vtkXMLMultiBlockDataReader()
read.SetFileName(file0)
read.Update()

output = read.GetOutputDataObject(0)
assert(output.GetFieldData().GetArray("foo"))
assert(output.GetFieldData().GetArray("foo").GetValue(0) == 10)

os.remove(file0)
os.remove(file_root + "/testxml_0.vti")
os.rmdir(file_root)
