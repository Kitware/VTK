package require vtk
package require vtkinteraction

# append multiple displaced spheres into an RGB image.


# Image pipeline

vtkImageWindow imgWin

vtkImageEllipsoidSource sphere1
sphere1 SetCenter 40 20 0
sphere1 SetRadius 30 30 0
sphere1 SetInValue .75
sphere1 SetOutValue .3
sphere1 SetOutputScalarTypeToFloat

vtkImageEllipsoidSource sphere2
sphere2 SetCenter 60 30 0
sphere2 SetRadius 20 20 20 
sphere2 SetInValue .2
sphere2 SetOutValue .5
sphere2 SetOutputScalarTypeToFloat

set mathematics "\
Add \
Subtract \
Multiply \
Divide \
Invert \
Sin \
Cos \
Exp \
Log \
AbsoluteValue \
Square \
SquareRoot \
Min \
Max \
ATAN \
ATAN2 \
MultiplyByK \
AddConstant"

foreach operator $mathematics {
    vtkImageMathematics mathematic${operator}
      mathematic${operator} SetInput1 [sphere1 GetOutput]
      mathematic${operator} SetInput2 [sphere2 GetOutput]
      mathematic${operator} SetOperationTo${operator}
      mathematic${operator} SetConstantK .21
      mathematic${operator} SetConstantC .1
    vtkImageMapper mapper${operator}
      mapper${operator} SetInput [mathematic${operator} GetOutput]
      mapper${operator} SetColorWindow 2.0
      mapper${operator} SetColorLevel .75
    vtkActor2D actor${operator}
      actor${operator} SetMapper mapper${operator}
    vtkImager imager${operator}
      imager${operator} AddActor2D actor${operator}
    imgWin AddImager imager${operator}
}

set column 1
set row 1
set deltaX [expr 1.0/6.0]
set deltaY [expr 1.0/3.0]

foreach operator $mathematics {
    imager${operator} SetViewport [expr ($column - 1) * $deltaX] [expr ($row - 1) * $deltaY] [expr $column * $deltaX] [expr $row * $deltaY]
    incr column
    if { $column > 6 } {set column 1; incr row}
}

imgWin SetSize 600 300
imgWin Render

wm withdraw .
