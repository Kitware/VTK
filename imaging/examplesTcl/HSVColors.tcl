# This example uses a programable source to create a ramp.
# This is converted into a volume (256x256x256) with 3 components.
# Each axis ramps independently.
# It is then converted into a color volume with Hue, 
# Saturation and Value ramping
catch {load vtktcl}

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source vtkImageInclude.tcl


vtkProgrammableSource rampSource
    rampSource SetExecuteMethod ramp

# Generate a single ramp value
proc ramp {} {
   vtkFloatScalars newScalars
   newScalars SetNumberOfScalars 256

   # Compute points and scalars
   for {set idx 0} {$idx < 256} {incr idx} {
      newScalars SetScalar $idx $idx
   }

   [rampSource GetStructuredPointsOutput] SetDimensions 256 1 1
   [[rampSource GetStructuredPointsOutput] GetPointData] SetScalars newScalars

    newScalars Delete; #reference counting - it's ok
}

# use pad filter to create a volume
vtkImageWrapPad pad
pad SetInput [rampSource GetStructuredPointsOutput]
pad SetOutputWholeExtent 0 255 0 255 0 255
pad ReleaseDataFlagOn


# hack work around of bug
vtkImageShiftScale copy1
copy1 SetInput [pad GetOutput]
vtkImageShiftScale copy2
copy2 SetInput [pad GetOutput]
vtkImageShiftScale copy3
copy3 SetInput [pad GetOutput]



# create HSV components
vtkImagePermute perm1
perm1 SetInput [copy1 GetOutput]
#perm1 SetInput [pad GetOutput]
perm1 SetFilteredAxes $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS $VTK_IMAGE_X_AXIS
perm1 ReleaseDataFlagOn
 
vtkImagePermute perm2
perm2 SetInput [copy2 GetOutput]
#perm2 SetInput [pad GetOutput]
perm2 SetFilteredAxes $VTK_IMAGE_Z_AXIS $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
perm2 ReleaseDataFlagOn

vtkImageAppendComponents append1
append1 SetInput1 [copy3 GetOutput]
#append1 SetInput1 [pad GetOutput]
append1 SetInput2 [perm1 GetOutput]
append1 ReleaseDataFlagOn

vtkImageAppendComponents append2
append2 SetInput1 [append1 GetOutput]
append2 SetInput2 [perm2 GetOutput]


vtkImageHSVToRGB rgb
    rgb SetInput [append2 GetOutput]
    rgb SetMaximum 255

vtkImageViewer viewer
viewer SetInput [rgb GetOutput]
viewer SetZSlice 128
viewer SetColorWindow 255
viewer SetColorLevel 128

# make interface
source WindowLevelInterface.tcl

