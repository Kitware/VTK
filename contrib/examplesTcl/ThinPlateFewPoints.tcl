catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# The ThinPlateSpline transform contains special code for handling
# cases where there are two, one, or no landmark points.
# Respectively, a rotation+scale+translation, a translation only, or
# the identity transform are applied instead of a thin plate spline
# in these cases

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 47 47
reader SetDataSpacing 1.0 1.0 2.0
reader SetDataOrigin 0.0 0.0 92.0
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

# no points
vtkPoints sp1
sp1 SetNumberOfPoints 0

vtkPoints tp1
tp1 SetNumberOfPoints 0 

vtkThinPlateSplineTransform thin1
thin1 SetSourceLandmarks sp1
thin1 SetTargetLandmarks tp1

# one point
vtkPoints sp2
sp2 SetNumberOfPoints 1
sp2 SetPoint 0 128 128 92

vtkPoints tp2
tp2 SetNumberOfPoints 1 
tp2 SetPoint 0 180 180 92

vtkThinPlateSplineTransform thin2
thin2 SetSourceLandmarks sp2
thin2 SetTargetLandmarks tp2

#two points
vtkPoints sp3
sp3 SetNumberOfPoints 2
sp3 SetPoint 0 128 128 92
sp3 SetPoint 1 128 148 92

vtkPoints tp3
tp3 SetNumberOfPoints 2 
tp3 SetPoint 0 128 148 92
tp3 SetPoint 1 148 168 92

vtkThinPlateSplineTransform thin3
thin3 SetSourceLandmarks sp3
thin3 SetTargetLandmarks tp3

# don't set anything at all
vtkThinPlateSplineTransform thin4

#-------------
vtkImageReslice reslice1
reslice1 SetInput [reader GetOutput]
reslice1 SetResliceTransform [thin1 GetInverse]
reslice1 SetInterpolationModeToLinear
reslice1 SetOutputSpacing 1.0 1.0 2.0
reslice1 SetOutputOrigin 0 0 0
reslice1 SetOutputExtent 0 255 0 255 0 92

vtkImageReslice reslice2
reslice2 SetInput [reader GetOutput]
reslice2 SetResliceTransform [thin2 GetInverse]
reslice2 SetInterpolationModeToLinear
reslice2 SetOutputSpacing 1.0 1.0 2.0
reslice2 SetOutputOrigin 0 0 0 
reslice2 SetOutputExtent 0 255 0 255 0 92

vtkImageReslice reslice3
reslice3 SetInput [reader GetOutput]
reslice3 SetResliceTransform [thin3 GetInverse]
reslice3 SetInterpolationModeToLinear
reslice3 SetOutputSpacing 1.0 1.0 2.0
reslice3 SetOutputOrigin 0 0 0 
reslice3 SetOutputExtent 0 255 0 255 0 92

vtkImageReslice reslice4
reslice4 SetInput [reader GetOutput]
reslice4 SetResliceTransform [thin4 GetInverse]
reslice4 SetInterpolationModeToLinear
reslice4 SetOutputSpacing 1.0 1.0 2.0
reslice4 SetOutputOrigin 0 0 0 
reslice4 SetOutputExtent 0 255 0 255 0 92

vtkImageMapper mapper1
  mapper1 SetInput [reslice1 GetOutput]
  mapper1 SetColorWindow 2000
  mapper1 SetColorLevel 1000
  mapper1 SetZSlice 46
#  mapper1 DebugOn

vtkImageMapper mapper2
  mapper2 SetInput [reslice2 GetOutput]
  mapper2 SetColorWindow 2000
  mapper2 SetColorLevel 1000
  mapper2 SetZSlice 46
#  mapper2 DebugOn

vtkImageMapper mapper3
  mapper3 SetInput [reslice3 GetOutput]
  mapper3 SetColorWindow 2000
  mapper3 SetColorLevel 1000
  mapper3 SetZSlice 46 
#  mapper3 DebugOn

vtkImageMapper mapper4
  mapper4 SetInput [reslice4 GetOutput]
  mapper4 SetColorWindow 2000
  mapper4 SetColorLevel 1000
  mapper4 SetZSlice 46
#  mapper4 DebugOn

vtkActor2D actor1
  actor1 SetMapper mapper1
#  actor1 DebugOn

vtkActor2D actor2
  actor2 SetMapper mapper2
#  actor2 DebugOn

vtkActor2D actor3
  actor3 SetMapper mapper3
#  actor3 DebugOn

vtkActor2D actor4
  actor4 SetMapper mapper4
#  actor4 DebugOn

vtkImager imager1
  imager1 AddActor2D actor1
  imager1 SetViewport 0.5 0.0 1.0 0.5
#  imager1 DebugOn

vtkImager imager2
  imager2 AddActor2D actor2
  imager2 SetViewport 0.0 0.0 0.5 0.5
#  imager2 DebugOn

vtkImager imager3
  imager3 AddActor2D actor3
  imager3 SetViewport 0.5 0.5 1.0 1.0
#  imager3 DebugOn

vtkImager imager4
  imager4 AddActor2D actor4
  imager4 SetViewport 0.0 0.5 0.5 1.0
#  imager4 DebugOn


vtkImageWindow imgWin
  imgWin AddImager imager1
  imgWin AddImager imager2
  imgWin AddImager imager3
  imgWin AddImager imager4
  imgWin SetSize 512 512
#  imgWin DebugOn

imgWin Render

wm withdraw .

vtkWindowToImageFilter w2i
w2i SetInput imgWin

vtkTIFFWriter tifWriter
tifWriter SetFileName ThinPlateFewPoints.tcl.tif
tifWriter SetInput [w2i GetOutput]
#tifWriter Write



