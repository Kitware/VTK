
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93 
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageMapper mapper2
  mapper2 SetInput [reader GetOutput]
  mapper2 SetColorWindow 2000
  mapper2 SetColorLevel 1000
  mapper2 SetZSlice 21
  mapper2 SetRenderToRectangle 1
  mapper2 SetUseCustomExtents 1
  mapper2 SetCustomDisplayExtents 128 147 128 147

vtkActor2D actor2
  actor2 SetMapper mapper2
  [actor2 GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport 
  [actor2 GetPositionCoordinate] SetValue 0.25 0.25
  [actor2 GetPosition2Coordinate] SetCoordinateSystemToNormalizedViewport
  [actor2 GetPosition2Coordinate] SetValue 0.5 0.5
# note that the topright corner is included in the image


vtkImager imager1
  imager1 AddActor2D actor2 

vtkImageWindow imgWin
  imgWin AddImager imager1
  imgWin Render
  imgWin SetSize 400 400

vtkWindowToImageFilter windowToImage
  windowToImage SetInput imgWin
vtkTIFFWriter tiffimage
  tiffimage SetInput [windowToImage GetOutput]
  tiffimage SetFileName TestRenderRectangle.tcl.tif
#  tiffimage Write

wm withdraw .

 
