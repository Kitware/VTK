
catch {load vtktcl}

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93 
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageMapper mapper2
  mapper2 SetInput [reader GetOutput]
  mapper2 SetColorWindow 2000
  mapper2 SetColorLevel 1000
  mapper2 SetZSlice 50 

vtkActor2D actor2
  actor2 SetMapper mapper2

vtkVectorText vtext;
  vtext SetText "Imagine!"

vtkTransform trans
trans Scale 25 25 25

vtkTransformPolyDataFilter tpd
tpd SetTransform trans
tpd SetInput [vtext GetOutput]

vtkPolyDataMapper2D textMapper
  textMapper SetInput [tpd GetOutput]

vtkCoordinate coord
coord SetCoordinateSystemToNormalizedViewport
coord SetValue 0.5 0.5

vtkActor2D textActor
  textActor SetMapper textMapper
[textActor GetProperty] SetColor 0.7 1.0 1.0
[textActor GetPositionCoordinate] SetReferenceCoordinate coord
[textActor GetPositionCoordinate] SetCoordinateSystemToViewport
[textActor GetPositionCoordinate] SetValue -80 -20

vtkImager imager1
  imager1 AddActor2D textActor

vtkImageWindow imgWin
  imgWin AddImager imager1


wm withdraw .
toplevel .top 
frame .top.f1 

vtkTkImageWindowWidget .top.f1.r1 -width 256 -height 256 -iw imgWin

button .top.btn  -text Quit -command exit

pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x

bind .top.f1.r1 <Expose> {.top.f1.r1 Render}

update

imgWin SetFileName "junk.ppm"
imgWin SaveImageAsPPM
exec rm junk.ppm
 
