catch {load vtktcl}
# Simple viewer for images.


source vtkImageInclude.tcl
source ../../examplesTcl/vtkInt.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn
#reader Update

vtkImageCacheFilter cache
cache SetInput [reader GetOutput]
cache SetCacheSize 20

vtkImageViewer viewer
viewer SetInput [cache GetOutput]
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer SetPosition 50 50
#viewer DebugOn

for {set i 0} {$i < 5} {incr i} {
  for {set j 10} {$j < 30} {incr j} {
     viewer SetZSlice $j
     viewer Render
  }
}


#wm deiconify .vtkInteract

#make interface
source WindowLevelInterface.tcl







