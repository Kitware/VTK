# This Script test the euclidean to polar by coverting 2D vectors 
# from a gradient into polar, which is converted into HSV, and then to RGB.
catch {load vtktcl}

source vtkImageInclude.tcl

vtkImageReader reader
    reader SetDataByteOrderToLittleEndian
    reader SetDataExtent 0 255 0 255 1 93
    reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
    reader SetDataMask 0x7fff
    reader SetOutputScalarTypeToFloat

vtkImageMagnify magnify
    magnify SetInput [reader GetOutput]
    magnify SetMagnificationFactors 4 4
    magnify SetFilteredAxes $VTK_IMAGE_Y_AXIS $VTK_IMAGE_X_AXIS
    magnify InterpolateOn

# remove high freqeuncy artifacts due to linear interpolation
vtkImageGaussianSmooth smooth
    smooth SetInput [magnify GetOutput]
    smooth SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
    smooth SetStandardDeviations 1.5 1.5
    smooth SetRadiusFactors 2.01 2.01

vtkImageGradient gradient
    gradient SetInput [smooth GetOutput]
    gradient SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
    gradient ReleaseDataFlagOff

vtkImageEuclideanToPolar polar
    polar SetInput [gradient GetOutput]
    polar SetThetaMaximum 255

vtkImageConstantPad pad
    pad SetInput [polar GetOutput]
    pad SetOutputNumberOfScalarComponents 3
    pad SetConstant 200

# permute components so saturation will be constant
vtkImageExtractComponents permute
    permute SetInput [pad GetOutput]
    permute SetComponents 0 2 1

vtkImageHSVToRGB rgb
    rgb SetInput [permute GetOutput]
    rgb SetMaximum 255

vtkImageViewer viewer
    viewer SetInput [rgb GetOutput]
    viewer SetZSlice 22
    viewer SetColorWindow 255
    viewer SetColorLevel 127

#make interface
source WindowLevelInterface.tcl







