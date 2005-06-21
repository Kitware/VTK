package require vtk


# Image pipeline
vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

vtkImageRange3D range
range SetInputConnection [reader GetOutputPort]
range SetKernelSize 5 5 5

vtkImageViewer viewer
viewer SetInputConnection [range GetOutputPort]
viewer SetZSlice 22
viewer SetColorWindow 1000
viewer SetColorLevel 500
#viewer DebugOn


viewer Render


