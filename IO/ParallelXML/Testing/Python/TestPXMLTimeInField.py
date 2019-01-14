#!/usr/bin/env python

import os
import vtk
from vtk.util.misc import vtkGetDataRoot
from vtk.util.misc import vtkGetTempDir

VTK_DATA_ROOT = vtkGetDataRoot()
VTK_TEMP_DIR = vtkGetTempDir()

rt = vtk.vtkRTAnalyticSource()
rt.Update()

inp = rt.GetOutput()
inp.GetInformation().Set(vtk.vtkDataObject.DATA_TIME_STEP(), 11)

file_root = VTK_TEMP_DIR + '/testpxmlfield'
file0 = file_root + ".pvti"

w = vtk.vtkXMLPImageDataWriter()
w.SetInputData(inp)
w.SetFileName(file0)
w.Write()

r = vtk.vtkXMLPImageDataReader()
r.SetFileName(file0)
r.UpdateInformation()
assert(r.GetOutputInformation(0).Get(vtk.vtkStreamingDemandDrivenPipeline.TIME_STEPS(), 0) == 11.0)

os.remove(file0)
os.remove(file_root+"_0.vti")
