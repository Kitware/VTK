catch {load vtktcl}
# Make an image larger by repeating the data.  Tile.


source vtkImageInclude.tcl


# Image pipeline

vtkImageVolume16Reader reader
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
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
viewer ColorFlagOn


# make interface
source WindowLevelInterface.tcl







