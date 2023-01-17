#!/usr/bin/env python

import os
from vtkmodules.vtkCommonDataModel import vtkDataObject
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
from vtkmodules.vtkIOXML import (
    vtkXMLImageDataReader,
    vtkXMLImageDataWriter,
)
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.util.misc import vtkGetDataRoot
from vtkmodules.util.misc import vtkGetTempDir

VTK_DATA_ROOT = vtkGetDataRoot()
VTK_TEMP_DIR = vtkGetTempDir()

rt = vtkRTAnalyticSource()
rt.Update()

inp = rt.GetOutput()
inp.GetInformation().Set(vtkDataObject.DATA_TIME_STEP(), 11)

file_root = VTK_TEMP_DIR + '/testxmlfield'
file0 = file_root + ".vti"

w = vtkXMLImageDataWriter()
w.SetInputData(inp)
w.SetFileName(file0)
w.Write()

r = vtkXMLImageDataReader()
r.SetFileName(file0)
r.UpdateInformation()
assert(r.GetOutputInformation(0).Get(vtkStreamingDemandDrivenPipeline.TIME_STEPS(), 0) == 11.0)

os.remove(file0)
