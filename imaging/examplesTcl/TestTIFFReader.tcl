catch {load vtktcl}

vtkTIFFReader reader
reader SetFileName "../../../vtkdata/testTIFF.tif"
#reader SetFileName $argv

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [reader GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 128

#make interface
source WindowLevelInterface.tcl


