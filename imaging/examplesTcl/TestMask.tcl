catch {load vtktcl}
# A script to test the mask filter.
# removes all but a sphere of headSq.

source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 3
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff
reader SetOutputScalarType $VTK_SHORT
#reader DebugOn

vtkImageSphereSource sphere
sphere SetWholeExtent 0 255 0 255 0 92
sphere SetCenter 128 128 46
sphere SetRadius 80

vtkImageMask mask
mask SetImageInput [reader GetOutput]
mask SetMaskInput [sphere GetOutput]
mask SetMaskedOutputValue 0.0;
mask NotMaskOn;
mask ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [mask GetOutput]
viewer SetZSlice 2
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







