catch {load vtktcl}
# Make an image larger by repeating the data.  Tile.


source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader ReleaseDataFlagOff
#reader DebugOn

vtkImageConstantPad pad
pad SetInput [reader GetOutput]
pad SetOutputNumberOfScalarComponents 3
pad SetConstant 800
pad ReleaseDataFlagOff


vtkImageViewer viewer
viewer SetInput [pad GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 1600
viewer SetColorLevel 700


# make interface
source WindowLevelInterface.tcl







