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

vtkImageWrapPad pad
pad SetInput [reader GetOutput]
pad SetOutputWholeExtent -200 355 -200 370 0 92
pad ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [pad GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000
[viewer GetActor2D] SetDisplayPosition 200 200

# make interface
source WindowLevelInterface.tcl



