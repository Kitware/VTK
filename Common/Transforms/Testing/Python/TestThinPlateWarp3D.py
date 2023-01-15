#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonTransforms import vtkThinPlateSplineTransform
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingCore import (
    vtkImageCacheFilter,
    vtkImageReslice,
)
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetDataOrigin(-100.8,-100.8,-69)
reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
reader.Update()
p1 = vtkPoints()
p2 = vtkPoints()
p1.InsertNextPoint(0,0,0)
p2.InsertNextPoint(-60,10,20)
p1.InsertNextPoint(-100,-100,-50)
p2.InsertNextPoint(-100,-100,-50)
p1.InsertNextPoint(-100,-100,50)
p2.InsertNextPoint(-100,-100,50)
p1.InsertNextPoint(-100,100,-50)
p2.InsertNextPoint(-100,100,-50)
p1.InsertNextPoint(-100,100,50)
p2.InsertNextPoint(-100,100,50)
p1.InsertNextPoint(100,-100,-50)
p2.InsertNextPoint(100,-100,-50)
p1.InsertNextPoint(100,-100,50)
p2.InsertNextPoint(100,-100,50)
p1.InsertNextPoint(100,100,-50)
p2.InsertNextPoint(100,100,-50)
p1.InsertNextPoint(100,100,50)
p2.InsertNextPoint(100,100,50)
transform = vtkThinPlateSplineTransform()
transform.SetSourceLandmarks(p1)
transform.SetTargetLandmarks(p2)
transform.SetBasisToR()
reslice = vtkImageReslice()
reslice.SetInputConnection(reader.GetOutputPort())
reslice.SetResliceTransform(transform)
reslice.SetInterpolationModeToLinear()
reslice.SetOutputSpacing(1,1,1)
cac = vtkImageCacheFilter()
cac.SetInputConnection(reslice.GetOutputPort())
cac.SetCacheSize(1000)
cac.SetInputConnection(reslice.GetOutputPort())
viewer = vtkImageViewer()
viewer.SetInputConnection(cac.GetOutputPort())
viewer.SetZSlice(90)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.SetSize(200,200)
viewer.Render()
# --- end of script --
