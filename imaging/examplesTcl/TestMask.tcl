catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# A script to test the mask filter.
# removes all but a sphere of headSq.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 94
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageEllipsoidSource sphere
sphere SetWholeExtent 0 255 0 255 1 94
sphere SetCenter 128 128 46
sphere SetRadius 80 80 80

vtkImageMask mask
mask SetImageInput [reader GetOutput]
mask SetMaskInput [sphere GetOutput]
mask SetMaskedOutputValue 500
mask NotMaskOn;
mask ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [mask GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







