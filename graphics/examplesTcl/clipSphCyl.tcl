catch {load vtktcl}
#
# Demonstrate the use of clipping on polygonal data
#
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# create pipeline
#
vtkPlaneSource plane
    plane SetXResolution 25
    plane SetYResolution 25
    plane SetOrigin -1 -1 0
    plane SetPoint1 1 -1 0
    plane SetPoint2 -1 1 0

vtkTransform transformSphere
  transformSphere Identity
  transformSphere Translate .4 -.4 0
  transformSphere Inverse

vtkSphere sphere
  sphere SetTransform transformSphere
  sphere SetRadius .5

vtkTransform transformCylinder
  transformCylinder Identity
  transformCylinder Translate -.4 .4 0
  transformCylinder RotateZ 30
  transformCylinder RotateY 60
  transformCylinder RotateX 90
  transformCylinder Inverse

vtkCylinder cylinder
  cylinder SetTransform transformCylinder
  cylinder SetRadius .3

vtkImplicitBoolean boolean
  boolean AddFunction cylinder
  boolean AddFunction sphere

vtkClipPolyData clipper
  clipper SetInput [plane GetOutput]
  clipper SetClipFunction boolean
  clipper GenerateClippedOutputOn
  clipper GenerateClipScalarsOn
  clipper SetValue 0

vtkPolyDataMapper clipMapper
    clipMapper SetInput [clipper GetOutput]
    clipMapper ScalarVisibilityOff

vtkActor clipActor
    clipActor SetMapper clipMapper
    eval [clipActor GetProperty] SetDiffuseColor $black
    eval [clipActor GetProperty] SetRepresentationToWireframe

vtkPolyDataMapper clipInsideMapper
    clipInsideMapper SetInput [clipper GetClippedOutput]
    clipInsideMapper ScalarVisibilityOff
vtkActor clipInsideActor
    clipInsideActor SetMapper clipInsideMapper
    eval [clipInsideActor GetProperty] SetDiffuseColor $dim_grey

# Create graphics stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor clipActor
#[clipActor GetProperty] SetWireframe

ren1 AddActor clipInsideActor
ren1 SetBackground 1 1 1
[ren1 GetActiveCamera] Dolly 1.5

renWin SetSize 512 512
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

#renWin SetFileName "clipSphCyl.tcl.ppm"
#renWin SaveImageAsPPM


