package require vtk

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetDataSpacing 3.2 3.2 1.5
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

vtkImageCacheFilter cache
cache SetInput [reader GetOutput]
cache SetCacheSize 20

vtkImageViewer viewer
viewer SetInput [cache GetOutput]
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer SetPosition 50 50

for {set i 0} {$i < 5} {incr i} {
  for {set j 10} {$j < 30} {incr j} {
     viewer SetZSlice $j
     viewer Render
  }
}


#make interface
viewer Render







