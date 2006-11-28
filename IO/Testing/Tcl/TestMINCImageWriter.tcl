package require vtk

# Image pipeline

vtkMINCImageReader reader
  reader SetFileName "$VTK_DATA_ROOT/Data/t3_grid_0.mnc"
  reader RescaleRealValuesOn

vtkMINCImageAttributes attributes

set image reader

#
# write to the temp directory if possible, otherwise use .
#
set dir "."
if {[info commands "rtTester"] == "rtTester"}  {
   set dir [rtTester GetTempDirectory]
}

# make sure it is writeable first
if {[catch {set channel [open "$dir/test.tmp" "w"]}] == 0 } {
   close $channel
   file delete -force "$dir/test.tmp"

   vtkMINCImageWriter minc1
   minc1 SetInputConnection [reader GetOutputPort]
   minc1 SetFileName "$dir/minc1.mnc"

   attributes ShallowCopy [reader GetImageAttributes]
   attributes SetAttributeValueAsString "patient" "full_name" "DOE^JOHN DAVID"

   vtkMINCImageWriter minc2
   minc2 SetImageAttributes attributes
   minc2 SetInputConnection [reader GetOutputPort]
   minc2 SetFileName "$dir/minc2.mnc"    

   vtkMINCImageWriter minc3
   minc3 SetImageAttributes attributes
   minc3 AddInputConnection [reader GetOutputPort]
   minc3 AddInputConnection [reader GetOutputPort]
   minc3 SetFileName "$dir/minc3.mnc"

   minc1 Write
   minc2 Write
   minc3 Write
   
   vtkMINCImageReader reader2
   reader2 SetFileName "$dir/minc3.mnc"
   reader2 RescaleRealValuesOn
   reader2 SetTimeStep 1
   [reader2 GetOutput] Update

   set image reader2

   file delete -force  "$dir/minc1.mnc"
   file delete -force  "$dir/minc2.mnc"
   file delete -force  "$dir/minc3.mnc"

   # write out the file header for coverage
   attributes PrintFileHeader
}

vtkImageViewer viewer
  viewer SetInputConnection [$image GetOutputPort]
  viewer SetColorWindow 100
  viewer SetColorLevel 0
  viewer Render

