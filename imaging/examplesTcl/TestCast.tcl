catch {load vtktcl}
# Test the vtkImageCast filter.
# Cast the shorts to unsinged chars.  This will cause overflow artifacts
# because the data does not fit into 8 bits.


source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader ReleaseDataFlagOff
#reader DebugOn

vtkImageCast cast
cast SetInput [reader GetOutput]
cast SetOutputScalarType $VTK_UNSIGNED_CHAR

vtkImageViewer viewer
viewer SetInput [cast GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 200
viewer SetColorLevel 60
#viewer DebugOn
viewer Render


# make interface
source WindowLevelInterface.tcl







