package require vtk


# Make an image larger by repeating the data.  Tile.



# Image pipeline

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetDataSpacing 3.2 3.2 1.5
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

vtkImageWrapPad pad
pad SetInput [reader GetOutput]
pad SetOutputWholeExtent -100 155 -100 170 0 92
pad ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [pad GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000
[viewer GetActor2D] SetDisplayPosition 150 150

viewer Render



