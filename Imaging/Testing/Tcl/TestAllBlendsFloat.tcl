package require vtk

# This script blends images that consist of float data

vtkRenderWindow imgWin
imgWin SetSize 512 256

# Image pipeline

vtkTIFFReader inputImage
  inputImage SetFileName "$VTK_DATA_ROOT/Data/beach.tif"

vtkBMPReader inputImage2
  inputImage2 SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"

# shrink the images to a reasonable size

vtkImageShrink3D shrink1
shrink1 SetInputConnection [inputImage GetOutputPort]
shrink1 SetShrinkFactors 2 2 1

vtkImageShrink3D shrink2
shrink2 SetInputConnection [inputImage2 GetOutputPort]
shrink2 SetShrinkFactors 2 2 1

vtkImageShiftScale color
color SetOutputScalarTypeToFloat
color SetShift 0
color SetScale [expr 1.0/255]
color SetInputConnection [shrink1 GetOutputPort]

vtkImageShiftScale backgroundColor
backgroundColor SetOutputScalarTypeToFloat
backgroundColor SetShift 0
backgroundColor SetScale [expr 1.0/255]
backgroundColor SetInputConnection [shrink2 GetOutputPort]

# create a greyscale version

vtkImageLuminance luminance
luminance SetInputConnection [color GetOutputPort]

vtkImageLuminance backgroundLuminance
backgroundLuminance SetInputConnection [backgroundColor GetOutputPort]

# create an alpha mask

vtkImageThreshold alpha
alpha SetInputConnection [luminance GetOutputPort]
alpha ThresholdByLower 0.9
alpha SetInValue 1.0
alpha SetOutValue 0.0

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
	mapper${row}${column} SetColorWindow 1.0
	mapper${row}${column} SetColorLevel 0.5
	
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






