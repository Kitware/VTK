catch {load vtktcl}

vtkTIFFReader reader
reader DebugOn
reader SetFileName "../../../vtkdata/testTIFF.tif"
#reader SetFileName "/home/hatfield/aaaaaaaa.cri"
#reader SetFileName "/home/dwaskill/wrk/cgsp/lorensen/ColorPlates/dentedKnee2.tif"
#reader SetFileName "/home/esopus/u1/lorensen/vtk/graphics/examplesTcl/walkCow.tif"
#reader SetFileName $argv

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [reader GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 128

#make interface
source WindowLevelInterface.tcl


