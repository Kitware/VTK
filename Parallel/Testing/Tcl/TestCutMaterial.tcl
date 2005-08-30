package require vtk
package require vtkinteraction


# Lets create a data set.
vtkImageData data
  data SetExtent 0 31 0 31 0 31
  data SetScalarTypeToFloat

# First the data array:
vtkImageGaussianSource gauss
  gauss SetWholeExtent 0 30 0 30 0 30
  gauss SetCenter 18 12 20
  gauss SetMaximum 1.0
  gauss SetStandardDeviation 10.0
  gauss Update

set a [[[gauss GetOutput] GetPointData] GetScalars]
$a SetName "Gauss"
[data GetCellData] SetScalars $a

gauss Delete


# Now the material array:
vtkImageEllipsoidSource ellipse
  ellipse SetWholeExtent 0 30 0 30 0 30
  ellipse SetCenter 11 12 13
  ellipse SetRadius 5 9 13
  ellipse SetInValue 1
  ellipse SetOutValue 0
  ellipse SetOutputScalarTypeToInt
  ellipse Update

set m [[[ellipse GetOutput] GetPointData] GetScalars]
$m SetName "Material"
[data GetCellData] AddArray $m

ellipse Delete

vtkCutMaterial cut
  cut SetInput data
  cut SetMaterialArrayName "Material"
  cut SetMaterial 1
  cut SetArrayName "Gauss"
  cut SetUpVector 1 0 0
  cut Update


vtkPolyDataMapper mapper2
mapper2 SetInputConnection [cut GetOutputPort]
mapper2 SetScalarRange 0 1
#apper2 SetScalarModeToUseCellFieldData
#apper2 SetColorModeToMapScalars 
#apper2 ColorByArrayComponent "vtkGhostLevels" 0

vtkActor actor2
actor2 SetMapper mapper2
actor2 SetPosition 1.5 0 0

vtkRenderer ren
ren AddActor actor2

vtkRenderWindow renWin
renWin AddRenderer ren

set p [cut GetCenterPoint]
set n [cut GetNormal]
set cam [ren GetActiveCamera]
eval $cam SetFocalPoint $p
eval $cam SetViewUp [cut GetUpVector]
$cam SetPosition [expr [lindex $n 0] + [lindex $p 0]] \
                 [expr [lindex $n 1] + [lindex $p 1]] \
                 [expr [lindex $n 2] + [lindex $p 2]]
ren ResetCamera




vtkRenderWindowInteractor iren
iren SetRenderWindow renWin
iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}

wm withdraw .

