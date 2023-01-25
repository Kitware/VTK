#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonTransforms import vtkThinPlateSplineTransform
from vtkmodules.vtkFiltersHybrid import (
    vtkGridTransform,
    vtkTransformToGrid,
)
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingCore import vtkImageReslice
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
gridThinPlate = vtkTransformToGrid()
gridThinPlate.SetInput(transform)
gridThinPlate.SetGridExtent(0,64,0,64,0,50)
gridThinPlate.SetGridSpacing(3.2,3.2,3.0)
gridThinPlate.SetGridOrigin(-102.4,-102.4,-75)
gridThinPlate.SetGridScalarTypeToUnsignedChar()
gridThinPlate.Update()
gridTransform = vtkGridTransform()
gridTransform.SetDisplacementGridData(gridThinPlate.GetOutput())
gridTransform.SetDisplacementShift(gridThinPlate.GetDisplacementShift())
gridTransform.SetDisplacementScale(gridThinPlate.GetDisplacementScale())
reslice = vtkImageReslice()
reslice.SetInputConnection(reader.GetOutputPort())
reslice.SetResliceTransform(gridTransform)
reslice.SetInterpolationModeToLinear()
reslice.SetOutputSpacing(1,1,1)
viewer = vtkImageViewer()
viewer.SetInputConnection(reslice.GetOutputPort())
viewer.SetZSlice(70)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.SetSize(200,200)
viewer.Render()
# --- end of script --
