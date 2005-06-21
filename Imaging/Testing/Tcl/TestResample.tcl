package require vtk

# Doubles the The number of images (x dimension).




# Image pipeline
vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetDataSpacing 3.2 3.2 1.5
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

vtkImageResample magnify
magnify SetDimensionality 3
magnify SetInputConnection [reader GetOutputPort]
magnify SetAxisOutputSpacing 0 0.52
magnify SetAxisOutputSpacing 1 2.2
magnify SetAxisOutputSpacing 2 0.8
magnify ReleaseDataFlagOff


vtkImageViewer viewer
viewer SetInputConnection [magnify GetOutputPort]
viewer SetZSlice 30
viewer SetColorWindow 2000
viewer SetColorLevel 1000

viewer Render




