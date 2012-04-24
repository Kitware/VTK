package require vtk



# Image pipeline
vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetDataSpacing 3.2 3.2 1.5
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

vtkImageVariance3D var
var SetInputConnection [reader GetOutputPort]
var SetKernelSize 3 3 1

vtkImageViewer viewer
viewer SetInputConnection [var GetOutputPort]
viewer SetZSlice 22
viewer SetColorWindow 3000
viewer SetColorLevel 1000
#viewer DebugOn

viewer Render


