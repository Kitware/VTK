package require vtk

# A script to test the mask filter.
#  replaces a circle with a color


# Image pipeline
vtkPNMReader reader
reader ReleaseDataFlagOff
reader SetFileName "$VTK_DATA_ROOT/Data/earth.ppm"

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

vtkImageEllipsoidSource sphere2
sphere2 SetWholeExtent 0 511 0 255 0 0
sphere2 SetCenter 328 128 0
sphere2 SetRadius 80 50 1

# Test the wrapping of the output masked value
vtkImageMask mask2
mask2 SetImageInput [mask GetOutput]
mask2 SetMaskInput [sphere2 GetOutput]
mask2 SetMaskedOutputValue 100;
mask2 NotMaskOn;
mask2 ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [mask2 GetOutput]
viewer SetColorWindow 255
viewer SetColorLevel 128
#viewer DebugOn

viewer Render







