# append multiple displaced spheres into an RGB image.
package require vtk


# Image pipeline

vtkRenderWindow imgWin

set logics "And Or Xor Nand Nor Not"
set types "Float Double UnsignedInt UnsignedLong UnsignedShort UnsignedChar"
set i 0
foreach operator $logics {
    set ScalarType [lindex $types $i]

    vtkImageEllipsoidSource sphere1${operator}
      sphere1${operator} SetCenter 95 100 0
      sphere1${operator} SetRadius 70 70 70
      sphere1${operator} SetOutputScalarTypeTo${ScalarType}

    vtkImageEllipsoidSource sphere2${operator}
      sphere2${operator} SetCenter 161 100 0
      sphere2${operator} SetRadius 70 70 70 
      sphere2${operator} SetOutputScalarTypeTo${ScalarType}

    vtkImageLogic logic${operator}
      logic${operator} SetInput1 [sphere1${operator} GetOutput]
      logic${operator} SetInput2 [sphere2${operator} GetOutput]
      logic${operator} SetOutputTrueValue 150
      logic${operator} SetOperationTo${operator}
    vtkImageMapper mapper${operator}
      mapper${operator} SetInput [logic${operator} GetOutput]
      mapper${operator} SetColorWindow 255
      mapper${operator} SetColorLevel 127.5
    vtkActor2D actor${operator}
      actor${operator} SetMapper mapper${operator}
    vtkRenderer imager${operator}
      imager${operator} AddActor2D actor${operator}
    imgWin AddRenderer imager${operator}
    incr i
}

imagerAnd SetViewport 0 .5 .33 1
imagerOr SetViewport .33 .5 .66 1
imagerXor SetViewport .66 .5 1 1
imagerNand SetViewport 0 0 .33 .5
imagerNor SetViewport .33 0 .66 .5
imagerNot SetViewport .66 0 1 .5

imgWin SetSize 768 512
imgWin Render

wm withdraw .
