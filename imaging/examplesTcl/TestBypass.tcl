catch {load vtktcl}
# This script is for testing the Nd Gaussian Smooth filter.

source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageGaussianSmooth smooth
smooth SetInput [reader GetOutput]
smooth SetDimensionality 2
smooth SetStandardDeviations 2 10
smooth BypassOn

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [smooth GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000

viewer Render
smooth BypassOff
viewer Render
smooth Modified
viewer Render
smooth Modified
viewer Render
smooth Modified
viewer Render

# make interface
source WindowLevelInterface.tcl

