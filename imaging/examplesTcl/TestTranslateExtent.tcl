catch {load vtktcl}
# Simple viewer for images.


source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn
#reader Update

vtkImageTranslateExtent translate
translate SetInput [reader GetOutput]
translate SetTranslation 10 10 10




vtkImageViewer viewer
viewer SetInput [translate GetOutput]
viewer SetZSlice 14
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn
viewer Render

#viewer SetPosition 50 50

#make interface
source WindowLevelInterface.tcl







