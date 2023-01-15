#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkFiltersParallelImaging import vtkMemoryLimitImageDataStreamer
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingCore import vtkImageMapToColors
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
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
# added these unused default arguments so that the prototype
# matches as required in python.
def changeLUT(*args):
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
mapToRGBA.AddObserver("EndEvent",changeLUT)
streamer = vtkMemoryLimitImageDataStreamer()
streamer.SetInputConnection(mapToRGBA.GetOutputPort())
streamer.SetMemoryLimit(100)
streamer.UpdateWholeExtent()
# set the window/level to 255.0/127.5 to view full range
viewer = vtkImageViewer()
viewer.SetInputConnection(streamer.GetOutputPort())
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
viewer.SetZSlice(50)
viewer.Render()
# --- end of script --
