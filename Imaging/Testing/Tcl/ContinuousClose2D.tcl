package require vtk


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

vtkImageContinuousDilate3D dilate
dilate SetInput [reader GetOutput]
dilate SetKernelSize 11 11 1

vtkImageContinuousErode3D erode
erode SetInput [dilate GetOutput]
erode SetKernelSize 11 11 1

vtkImageViewer viewer
viewer SetInput [erode GetOutput]
viewer SetColorWindow 2000
viewer SetColorLevel 1000


viewer Render


