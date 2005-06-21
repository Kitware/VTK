package require vtk
package require vtkinteraction

# Image pipeline

vtkRenderWindow imgWin

vtkImageCanvasSource2D image1
  image1 SetNumberOfScalarComponents 3
  image1 SetScalarTypeToUnsignedChar
  image1 SetExtent 0 79 0 79 0 0
  image1 SetDrawColor 255 255 0
  image1 FillBox 0 79 0 79

vtkImageCanvasSource2D image2
  image2 SetNumberOfScalarComponents 3
  image2 SetScalarTypeToUnsignedChar
  image2 SetExtent 0 79 0 79 0 0
  image2 SetDrawColor 0 255 255
  image2 FillBox 0 79 0 79

vtkImageMapper mapper
  mapper SetInputConnection [image1 GetOutputPort]
  mapper SetColorWindow 255
  mapper SetColorLevel 127.5
vtkActor2D actor
  actor SetMapper mapper
vtkRenderer imager
  imager AddActor2D actor

imgWin AddRenderer imager

set wipes "Quad Horizontal Vertical LowerLeft LowerRight UpperLeft UpperRight"

foreach wipe $wipes {

    vtkImageRectilinearWipe wiper${wipe}
      wiper${wipe} SetInput 0 [image1 GetOutput]
      wiper${wipe} SetInput 1 [image2 GetOutput]
      wiper${wipe} SetPosition 20 20
      wiper${wipe} SetWipeTo${wipe}
  
    vtkImageMapper mapper${wipe}
      mapper${wipe} SetInputConnection [wiper${wipe} GetOutputPort]
      mapper${wipe} SetColorWindow 255
      mapper${wipe} SetColorLevel 127.5
    vtkActor2D actor${wipe}
      actor${wipe} SetMapper mapper${wipe}
    vtkRenderer imager${wipe}
      imager${wipe} AddActor2D actor${wipe}

    imgWin AddRenderer imager${wipe}
}
imagerQuad SetViewport 0 .5 .25 1
imagerHorizontal SetViewport .25 .5 .5 1
imagerVertical SetViewport .5 .5 .75 1
imagerLowerLeft SetViewport .75 .5 1 1
imagerLowerRight SetViewport 0 0 .25 .5
imagerUpperLeft SetViewport .25 0 .5 .5
imagerUpperRight SetViewport .5 0 .75 .5
imager SetViewport .75 0 1 .5

imgWin SetSize 400 200
imgWin Render

wm withdraw .
