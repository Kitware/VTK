# This Script test the euclidean to polar by coverting 2D vectors 
# from a gradient into polar, which is converted into HSV, and then to RGB.
catch {load vtktcl}

source vtkImageInclude.tcl

vtkImageReader reader
    reader SetDataByteOrderToLittleEndian
    reader SetDataExtent 0 255 0 255 1 93
    reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
    reader SetDataMask 0x7fff

vtkImageCast cast
cast SetInput [reader GetOutput]
cast SetOutputScalarTypeToFloat

vtkImageMagnify magnify
    magnify SetInput [cast GetOutput]
    magnify SetMagnificationFactors 4 4 1
    magnify InterpolateOn

# remove high freqeuncy artifacts due to linear interpolation
vtkImageGaussianSmooth smooth
    smooth SetInput [magnify GetOutput]
    smooth SetDimensionality 2
    smooth SetStandardDeviations 1.5 1.5 0
    smooth SetRadiusFactors 2.01 2.01 0

vtkImageGradient gradient
    gradient SetInput [smooth GetOutput]
    gradient SetDimensionality 2

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







