#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkLookupTable,
    vtkMultiThreader,
)
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingCore import (
    vtkImageDataStreamer,
    vtkImageMapToColors,
)
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

vtkMultiThreader.SetGlobalMaximumNumberOfThreads(1)

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
rangeStart = 0.0
rangeEnd = 0.2
LUT = vtkLookupTable()
LUT.SetTableRange(0,1800)
LUT.SetSaturationRange(1,1)
LUT.SetHueRange(rangeStart,rangeEnd)
LUT.SetValueRange(1,1)
LUT.SetAlphaRange(1,1)
LUT.Build()
# The prototype of this function has
# arguments so that the code can be
# translated to python for testing.
def changeLUT(obj=None, event=""):
    global rangeStart, rangeEnd
    rangeStart += 0.1
    rangeEnd += 0.1
    if rangeEnd > 1.0:
        rangeStart = 0.0
        rangeEnd = 0.2
    LUT.SetHueRange(rangeStart,rangeEnd)
    LUT.Build()

mapToRGBA = vtkImageMapToColors()
mapToRGBA.SetInputConnection(reader.GetOutputPort())
mapToRGBA.SetOutputFormatToRGBA()
mapToRGBA.SetLookupTable(LUT)
#mapToRGBA.AddObserver("EndEvent",changeLUT)
imageStreamer = vtkImageDataStreamer()
imageStreamer.SetInputConnection(mapToRGBA.GetOutputPort())
imageStreamer.SetNumberOfStreamDivisions(8)
# make sure we get the correct translator.
imageStreamer.UpdateInformation()
imageStreamer.GetExtentTranslator().SetSplitModeToBlock()
# set the window/level to 255.0/127.5 to view full range
viewer = vtkImageViewer()
viewer.SetInputConnection(imageStreamer.GetOutputPort())
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
viewer.SetZSlice(50)
viewer.Render()
#make interface
viewer.Render()
# --- end of script --
