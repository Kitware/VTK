# append multiple displaced spheres into an RGB image.
catch {load vtktcl}

# Image pipeline

vtkImageWindow imgWin

vtkImageEllipsoidSource sphere1
sphere1 SetCenter 95 100 0
sphere1 SetRadius 70 70 70

vtkImageEllipsoidSource sphere2
sphere2 SetCenter 161 100 0
sphere2 SetRadius 70 70 70 

set logics "And Or Xor Nand Nor Not"
foreach operator $logics {
    vtkImageLogic logic${operator}
      logic${operator} SetInput1 [sphere1 GetOutput]
      logic${operator} SetInput2 [sphere2 GetOutput]
      logic${operator} SetOutputTrueValue 150
      logic${operator} SetOperationTo${operator}
    vtkImageMapper mapper${operator}
      mapper${operator} SetInput [logic${operator} GetOutput]
      mapper${operator} SetColorWindow 255
      mapper${operator} SetColorLevel 127.5
    vtkActor2D actor${operator}
      actor${operator} SetMapper mapper${operator}
    vtkImager imager${operator}
      imager${operator} AddActor2D actor${operator}
    imgWin AddImager imager${operator}
}

imagerAnd SetViewport 0 .5 .33 1
imagerOr SetViewport .33 .5 .66 1
imagerXor SetViewport .66 .5 1 1
imagerNand SetViewport 0 0 .33 .5
imagerNor SetViewport .33 0 .66 .5
imagerNot SetViewport .66 0 1 .5

imgWin SetSize 768 512
imgWin Render
imgWin SetFileName TestAllLogic.tcl.ppm
#imgWin SaveImageAsPPM

wm withdraw .
