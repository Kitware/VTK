catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


set prefix "$VTK_DATA/fullHead/headsq"

vtkImageWindow imgWin

# Image pipeline
vtkImageReader reader
  reader SetDataExtent 0 255 0 255 1 93
  reader SetFilePrefix $prefix
  reader SetDataByteOrderToLittleEndian
  reader SetDataMask 0x7fff

set factor 6

set ops "Minimum Maximum Mean Median"
foreach operator $ops {
  vtkImageShrink3D shrink${operator}
    shrink${operator} ${operator}On
    eval shrink${operator} SetShrinkFactors 1 1 93
  shrink${operator} SetInput [reader GetOutput];
  vtkImageMagnify mag${operator}
    mag${operator} SetMagnificationFactors 1 1 1
    mag${operator} InterpolateOff
    mag${operator} SetInput [shrink${operator} GetOutput]
  vtkImageMapper mapper${operator}
    mapper${operator} SetInput [mag${operator} GetOutput]
    mapper${operator} SetColorWindow 2000
    mapper${operator} SetColorLevel 1000
    mapper${operator} SetZSlice 0
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

imgWin SetSize 512 512
imgWin Render
imgWin SetFileName TestAllShrinks.tcl.ppm
#imgWin SaveImageAsPPM
wm withdraw .
