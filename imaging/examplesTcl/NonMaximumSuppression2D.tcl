# This script is for testing the 2dNonMaximumSuppressionFilter.
# The filter works exclusively on the output of the gradient filter.
# The effect is to pick the peaks of the gradient creating thin lines.

catch {load vtktcl}
source vtkImageInclude.tcl



# Image pipeline
vtkImageVolume16Reader reader
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageGradient gradient
gradient SetInput [reader GetOutput]
gradient SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
gradient ReleaseDataFlagOff

vtkImageMagnitude magnitude
magnitude SetInput [gradient GetOutput]

vtkImageNonMaximumSuppression suppress
suppress SetVectorInput [gradient GetOutput]
suppress SetMagnitudeInput [magnitude GetOutput]
suppress SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
suppress ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [suppress GetOutput]
viewer SetCoordinate2 22
viewer SetColorWindow 1000
viewer SetColorLevel 500
#viewer DebugOn
viewer Render


# make interface
source WindowLevelInterface.tcl

