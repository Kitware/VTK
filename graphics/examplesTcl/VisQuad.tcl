catch {load vtktcl}
source ../../examplesTcl/vtkInt.tcl

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Quadric definition
  vtkQuadric quadric
    quadric SetCoefficients .5 1 .2 0 .1 0 0 .2 0 0

  vtkSampleFunction sample
    sample SetSampleDimensions 30 30 30
    sample SetImplicitFunction quadric
    sample Update
    sample Print
    
# Create five surfaces F(x,y,z) = constant between range specified
  vtkContourFilter contours
    contours SetInput [sample GetOutput]
    contours GenerateValues 5 0.0 1.2

  vtkPolyDataMapper contMapper
    contMapper SetInput [contours GetOutput]
    contMapper SetScalarRange 0.0 1.2

  vtkActor contActor
    contActor SetMapper contMapper

# Create outline
  vtkOutlineFilter outline
    outline SetInput [sample GetOutput]

  vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]

  vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor 0 0 0

  ren1 SetBackground 1 1 1
  ren1 AddActor contActor
  ren1 AddActor outlineActor

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName VisQuad.tcl.ppm
#renWin SaveImageAsPPM

wm withdraw .
