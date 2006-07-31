package require vtk

# This script calculates the luminance of an image

vtkRenderWindow imgWin
imgWin SetSize 512 256

# Image pipeline

vtkTIFFReader image1
  image1 SetFileName "$VTK_DATA_ROOT/Data/beach.tif"

vtkBMPReader image2
  image2 SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"

# shrink the images to a reasonable size

vtkImageShrink3D color
color SetInputConnection [image1 GetOutputPort]
color SetShrinkFactors 2 2 1

vtkImageShrink3D backgroundColor
backgroundColor SetInputConnection [image2 GetOutputPort]
backgroundColor SetShrinkFactors 2 2 1

# create a greyscale version

vtkImageLuminance luminance
luminance SetInputConnection [color GetOutputPort]

vtkImageLuminance backgroundLuminance
backgroundLuminance SetInputConnection [backgroundColor GetOutputPort]

# create an alpha mask

vtkLookupTable table
table SetTableRange 220 255
table SetValueRange 1 0
table SetSaturationRange 0 0
table Build

vtkImageMapToColors alpha
alpha SetInputConnection [luminance GetOutputPort]
alpha SetLookupTable table
alpha SetOutputFormatToLuminance

# make luminanceAlpha and colorAlpha versions 

vtkImageAppendComponents luminanceAlpha
luminanceAlpha AddInput [luminance GetOutput]
luminanceAlpha AddInput [alpha GetOutput]

vtkImageAppendComponents colorAlpha
colorAlpha AddInput [color GetOutput]
colorAlpha AddInput [alpha GetOutput]

set foregrounds "luminance luminanceAlpha color colorAlpha"
set backgrounds "backgroundColor backgroundLuminance"

set column 1
set row 1
set deltaX [expr 1.0/4.0]
set deltaY [expr 1.0/2.0]

foreach background $backgrounds {
    foreach foreground $foregrounds {
	vtkImageBlend blend${row}${column}
	blend${row}${column} AddInput [$background GetOutput]
	if { $background == "backgroundColor" || $foreground == "luminance" || $foreground == "luminanceAlpha" } { 
	    blend${row}${column} AddInput [$foreground GetOutput]
	    blend${row}${column} SetOpacity 1 0.8
	}

	vtkImageMapper mapper${row}${column}
	mapper${row}${column} SetInputConnection [blend${row}${column} GetOutputPort]
	mapper${row}${column} SetColorWindow 255
	mapper${row}${column} SetColorLevel 127.5

	vtkActor2D actor${row}${column}
	actor${row}${column} SetMapper mapper${row}${column}

	vtkRenderer imager${row}${column}
	imager${row}${column} AddActor2D actor${row}${column}

	imager${row}${column} SetViewport [expr ($column - 1) * $deltaX] [expr ($row - 1) * $deltaY] [expr $column * $deltaX] [expr $row * $deltaY]

	imgWin AddRenderer imager${row}${column}

	incr column
    }
    incr row
    set column 1
}

imgWin Render

wm withdraw .






