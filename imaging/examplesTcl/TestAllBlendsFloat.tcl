catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# This script blends images that consist of float data

vtkImageWindow imgWin
imgWin SetSize 512 256

# Image pipeline

vtkBMPReader image
  image SetFileName "$VTK_DATA/beach.bmp"

vtkPNMReader image2
  image2 SetFileName "$VTK_DATA/masonry.ppm"

# shrink the images to a reasonable size

vtkImageShrink3D shrink1
shrink1 SetInput [image GetOutput]
shrink1 SetShrinkFactors 4 4 1

vtkImageShrink3D shrink2
shrink2 SetInput [image2 GetOutput]
shrink2 SetShrinkFactors 2 2 1

vtkImageShiftScale color
color SetOutputScalarTypeToFloat
color SetShift 0
color SetScale [expr 1.0/255]
color SetInput [shrink1 GetOutput]

vtkImageShiftScale backgroundColor
backgroundColor SetOutputScalarTypeToFloat
backgroundColor SetShift 0
backgroundColor SetScale [expr 1.0/255]
backgroundColor SetInput [shrink2 GetOutput]

# create a greyscale version

vtkImageLuminance luminance
luminance SetInput [color GetOutput]

vtkImageLuminance backgroundLuminance
backgroundLuminance SetInput [backgroundColor GetOutput]

# create an alpha mask

vtkImageThreshold alpha
alpha SetInput [luminance GetOutput]
alpha ThresholdByLower 0.9
alpha SetInValue 1.0
alpha SetOutValue 0.0

# make luminanceAlpha and colorAlpha versions 

vtkImageAppendComponents luminanceAlpha
luminanceAlpha SetInput1 [luminance GetOutput]
luminanceAlpha SetInput2 [alpha GetOutput]

vtkImageAppendComponents colorAlpha
colorAlpha SetInput1 [color GetOutput]
colorAlpha SetInput2 [alpha GetOutput]

set foregrounds "luminance luminanceAlpha color colorAlpha"
set backgrounds "backgroundColor backgroundLuminance"

set column 1
set row 1
set deltaX [expr 1.0/4.0]
set deltaY [expr 1.0/2.0]

foreach background $backgrounds {
    foreach foreground $foregrounds {
	vtkImageBlend blend${row}${column}
	blend${row}${column} SetInput 0 [$background GetOutput]
	if { $background == "backgroundColor" || $foreground == "luminance" || $foreground == "luminanceAlpha" } { 
	    blend${row}${column} SetInput 1 [$foreground GetOutput]
	    blend${row}${column} SetOpacity 1 0.8
	}

	vtkImageMapper mapper${row}${column}
	mapper${row}${column} SetInput [blend${row}${column} GetOutput]
	mapper${row}${column} SetColorWindow 1.0
	mapper${row}${column} SetColorLevel 0.5
	
	vtkActor2D actor${row}${column}
	actor${row}${column} SetMapper mapper${row}${column}
	
	vtkImager imager${row}${column}
	imager${row}${column} AddActor2D actor${row}${column}

	imager${row}${column} SetViewport [expr ($column - 1) * $deltaX] [expr ($row - 1) * $deltaY] [expr $column * $deltaX] [expr $row * $deltaY]

	imgWin AddImager imager${row}${column}

	incr column
    }
    incr row
    set column 1
}

imgWin Render

vtkWindowToImageFilter windowToimage
  windowToimage SetInput imgWin

vtkTIFFWriter tiffWriter
  tiffWriter SetInput [windowToimage GetOutput]
  tiffWriter SetFileName "TestAllBlendsFloat.tcl.tif"
#  tiffWriter Write

wm withdraw .






