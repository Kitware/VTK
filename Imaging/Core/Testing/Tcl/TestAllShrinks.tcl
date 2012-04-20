package require vtk

set prefix "$VTK_DATA_ROOT/Data/headsq/quarter"

vtkRenderWindow imgWin

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
    shrink${operator} SetMean 0
   shrink${operator} ${operator}On
    eval shrink${operator} SetShrinkFactors $factor $factor $factor
  shrink${operator} SetInputConnection [reader GetOutputPort];
  vtkImageMagnify mag${operator}
    mag${operator} SetMagnificationFactors $magFactor $magFactor $magFactor;
    mag${operator} InterpolateOff
    mag${operator} SetInputConnection [shrink${operator} GetOutputPort]
  vtkImageMapper mapper${operator}
    mapper${operator} SetInputConnection [mag${operator} GetOutputPort]
    mapper${operator} SetColorWindow 2000
    mapper${operator} SetColorLevel 1000
    mapper${operator} SetZSlice 45
  vtkActor2D actor${operator}
    actor${operator} SetMapper mapper${operator}
  vtkRenderer imager${operator}
    imager${operator} AddActor2D actor${operator}
  imgWin AddRenderer imager${operator}
}

  vtkImageShrink3D shrink
    shrink SetMean 0
    eval shrink SetShrinkFactors $factor $factor $factor
  shrink SetInputConnection [reader GetOutputPort];
  vtkImageMagnify mag
    mag SetMagnificationFactors $magFactor $magFactor $magFactor;
    mag InterpolateOff
    mag SetInputConnection [shrink GetOutputPort]
  vtkImageMapper mapper
    mapper SetInputConnection [mag GetOutputPort]
    mapper SetColorWindow 2000
    mapper SetColorLevel 1000
    mapper SetZSlice 45
  vtkActor2D actor
    actor SetMapper mapper
  vtkRenderer imager
    imager AddActor2D actor
  imgWin AddRenderer imager

#shrinkMinimum Update
#shrinkMaximum Update
#shrinkMean Update
#shrinkMedian Update

imagerMinimum SetViewport 0 0 .5 .33
imagerMaximum SetViewport 0 .33 .5 .667
imagerMean SetViewport .5 0 1 .33
imagerMedian SetViewport .5 .33 1 .667
imager SetViewport 0 .667 1 1

imgWin SetSize 256 384
imgWin Render

wm withdraw .
