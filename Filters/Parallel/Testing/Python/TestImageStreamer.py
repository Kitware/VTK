#!/usr/bin/env python

reader = vtk.vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
rangeStart = 0.0
rangeEnd = 0.2
LUT = vtk.vtkLookupTable()
LUT.SetTableRange(0,1800)
LUT.SetSaturationRange(1,1)
LUT.SetHueRange(rangeStart,rangeEnd)
LUT.SetValueRange(1,1)
LUT.SetAlphaRange(1,1)
LUT.Build()
# added these unused default arguments so that the prototype
# matches as required in python.
def changeLUT (a=0,b=0,__vtk__temp0=0,__vtk__temp1=0):
    global rangeStart, rangeEnd
    rangeStart = expr.expr(globals(), locals(),["rangeStart","+","0.1"])
    rangeEnd = expr.expr(globals(), locals(),["rangeEnd","+","0.1"])
    if (rangeEnd > 1.0):
        rangeStart = 0.0
        rangeEnd = 0.2
        pass
    LUT.SetHueRange(rangeStart,rangeEnd)
    LUT.Build()

mapToRGBA = vtk.vtkImageMapToColors()
mapToRGBA.SetInputConnection(reader.GetOutputPort())
mapToRGBA.SetOutputFormatToRGBA()
mapToRGBA.SetLookupTable(LUT)
mapToRGBA.AddObserver("EndEvent",changeLUT)
streamer = vtk.vtkMemoryLimitImageDataStreamer()
streamer.SetInputConnection(mapToRGBA.GetOutputPort())
streamer.SetMemoryLimit(100)
streamer.UpdateWholeExtent()
# set the window/level to 255.0/127.5 to view full range
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(streamer.GetOutputPort())
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
viewer.SetZSlice(50)
viewer.Render()
# --- end of script --
