
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
  vtext SetText "VTK Baby!"

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
[textActor GetProperty] SetColor 0.7 0.7 1.0
[textActor GetPositionCoordinate] SetReferenceCoordinate coord
[textActor GetPositionCoordinate] SetCoordinateSystemToViewport
[textActor GetPositionCoordinate] SetValue -100 -20

textActor SetScale 20 20 

vtkImager imager1
  imager1 AddActor2D textActor

vtkImageWindow imgWin
  imgWin AddImager imager1
  imgWin Render

wm withdraw .

 
