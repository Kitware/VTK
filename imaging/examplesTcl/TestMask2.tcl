catch {load vtktcl}
# A script to test the mask filter.
#  replaces a circle with a color

source vtkImageInclude.tcl

# Image pipeline
vtkPNMReader reader
reader ReleaseDataFlagOff
reader SetFileName "../../../vtkdata/earth.ppm"

vtkImageEllipsoidSource sphere
sphere SetWholeExtent 0 511 0 255 0 0
sphere SetCenter 128 128 0
sphere SetRadius 80 80 1

vtkImageMask mask
mask SetImageInput [reader GetOutput]
mask SetMaskInput [sphere GetOutput]
mask SetMaskedOutputValue 100 128 200;
mask NotMaskOn;
mask ReleaseDataFlagOff

puts [mask Print]

vtkImageViewer viewer
viewer SetInput [mask GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 255
viewer SetColorLevel 128
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







