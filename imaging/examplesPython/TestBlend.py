from vtkpython import *
from WindowLevelInterface import *

# do alpha-blending of two images

reader1 = vtkPNMReader()
reader1.SetFileName("../../../vtkdata/masonry.ppm")

reader2 = vtkPNMReader()
reader2.SetFileName("../../../vtkdata/B.pgm")

table = vtkLookupTable()
table.SetTableRange(0,127)
table.SetValueRange(0.0,1.0)
table.SetSaturationRange(0.0,0.0)
table.SetHueRange(0.0,0.0)
table.SetAlphaRange(0.9,0.0)
table.Build()

rgba = vtkImageMapToColors()
rgba.SetInput(reader2.GetOutput())
rgba.SetLookupTable(table)
             
blend = vtkImageBlend()
blend.SetInput(0,reader1.GetOutput())
blend.SetInput(1,rgba.GetOutput())
#blend.SetOpacity(1,0.5)

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(blend.GetOutput())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)

viewer.Render()

windowToimage = vtkWindowToImageFilter()
windowToimage.SetInput(viewer.GetImageWindow())

pnmWriter = vtkPNMWriter()
pnmWriter.SetInput(windowToimage.GetOutput())
pnmWriter.SetFileName("TestBlendRGBA.tcl.ppm")
#pnmWriter.Write()

#make interface
WindowLevelInterface(viewer)
