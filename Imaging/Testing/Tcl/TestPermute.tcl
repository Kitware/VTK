package require vtk

# Simple viewer for images.



# Image pipeline
vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

vtkImagePermute permute
permute SetInput [reader GetOutput]
permute SetFilteredAxes 1 2 0

vtkImageViewer viewer
viewer SetInput [permute GetOutput]
viewer SetZSlice 32
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn
#viewer Render

#make interface
viewer Render







