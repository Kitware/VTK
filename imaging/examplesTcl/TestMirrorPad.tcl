catch {load vtktcl}
# Make an image larger by repeating the data.  Tile.


source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 94
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader ReleaseDataFlagOff
#reader DebugOn

vtkImageMirrorPad pad
pad SetInput [reader GetOutput]
pad SetOutputWholeExtent -512 350 -512 350 1 94

vtkImageViewer viewer
viewer SetInput [pad GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000

#make interface
source WindowLevelInterface.tcl







