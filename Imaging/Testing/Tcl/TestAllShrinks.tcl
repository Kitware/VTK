package require vtk

set prefix "$VTK_DATA_ROOT/Data/headsq/quarter"

vtkImageWindow imgWin

# Image pipeline
vtkImageReader reader
  reader SetDataExtent 0 63 0 63 1 93
  reader SetFilePrefix $prefix
  reader SetDataByteOrderToLittleEndian
  reader SetDataMask 0x7fff

set factor 4
set magFactor 8

set ops "Minimum Maximum Mean Median"
foreach operator $ops {
  vtkImageShrink3D shrink${operator}
    shrink${operator} ${operator}On
    eval shrink${operator} SetShrinkFactors $factor $factor $factor
  shrink${operator} SetInput [reader GetOutput];
  vtkImageMagnify mag${operator}
    mag${operator} SetMagnificationFactors $magFactor $magFactor $magFactor;
    mag${operator} InterpolateOff
    mag${operator} SetInput [shrink${operator} GetOutput]
  vtkImageMapper mapper${operator}
    mapper${operator} SetInput [mag${operator} GetOutput]
    mapper${operator} SetColorWindow 2000
    mapper${operator} SetColorLevel 1000
    mapper${operator} SetZSlice 45
  vtkActor2D actor${operator}
    actor${operator} SetMapper mapper${operator}
  vtkImager imager${operator}
    imager${operator} AddActor2D actor${operator}
  imgWin AddImager imager${operator}
}


#shrinkMinimum Update
#shrinkMaximum Update
#shrinkMean Update
#shrinkMedian Update

imagerMinimum SetViewport 0 0 .5 .5
imagerMaximum SetViewport 0 .5 .5 1
imagerMean SetViewport .5 0 1 .5
imagerMedian SetViewport .5 .5 1 1

imgWin SetSize 256 256
imgWin Render

wm withdraw .
