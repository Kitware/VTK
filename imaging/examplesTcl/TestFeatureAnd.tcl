catch {load vtktcl}
# A script to test the island removal filter.
# first the image is thresholded, then small islands are removed.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 0 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader SetOutputScalarType $VTK_SHORT
#reader DebugOn

vtkImageThreshold thresh1
thresh1 SetInput [reader GetOutput]
thresh1 SetOutputScalarType $VTK_UNSIGNED_CHAR
thresh1 ThresholdByUpper 2000.0
thresh1 SetInValue 255
thresh1 SetOutValue 0
thresh1 ReleaseDataFlagOff

vtkImageThreshold thresh2
thresh2 SetInput [reader GetOutput]
thresh2 SetOutputScalarType $VTK_UNSIGNED_CHAR
thresh2 ThresholdByUpper 2700.0
thresh2 SetInValue 255
thresh2 SetOutValue 0
thresh2 ReleaseDataFlagOff

vtkImageFeatureAnd connect
connect SetInput1 [thresh1 GetOutput]
connect SetInput2 [thresh2 GetOutput]
connect SetOutputConnectedValue 255
connect SetOutputUnconnectedValue 128
connect SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
#connect SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS 

vtkImageViewer viewer
viewer SetInput [connect GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 255
viewer SetColorLevel 127.5
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







