#
# use implicit modeller to create a logo
#

# get the interactor ui
source ../../../examplesTcl/vtkInt.tcl

#
# get some nice colors
source ../../../examplesTcl/colors.tcl

# define some renderman shaders
source marble.tcl
source dented.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# make the letter v
vtkVectorText letterV
  letterV SetText v

# read the geometry file containing the letter t
vtkVectorText letterT
  letterT SetText t

# read the geometry file containing the letter k
vtkVectorText letterK
  letterK SetText k

# create a transform and transform filter for each letter
vtkTransform VTransform
vtkTransformPolyDataFilter VTransformFilter
  VTransformFilter SetInput [letterV GetOutput ]
  VTransformFilter SetTransform VTransform

vtkTransform TTransform
vtkTransformPolyDataFilter TTransformFilter
  TTransformFilter SetInput [letterT GetOutput ]
  TTransformFilter SetTransform TTransform

vtkTransform KTransform
vtkTransformPolyDataFilter KTransformFilter
  KTransformFilter SetInput [letterK GetOutput ]
  KTransformFilter SetTransform KTransform

# now append them all
vtkAppendPolyData appendAll
  appendAll AddInput [VTransformFilter GetOutput ]
  appendAll AddInput [TTransformFilter GetOutput ]
  appendAll AddInput [KTransformFilter GetOutput ]

# create normals
vtkPolyDataNormals logoNormals
  logoNormals SetInput [appendAll GetOutput ]
  logoNormals SetFeatureAngle 60

# map to rendering primitives
vtkPolyDataMapper logoMapper
  logoMapper SetInput [logoNormals GetOutput ]

# now an actor
vtkActor logo
  logo SetMapper logoMapper
eval logo SetProperty [eval marble { 50 $ivory $black }]

# now create an implicit model of the same letter
vtkImplicitModeller blobbyLogoImp
blobbyLogoImp SetInput [appendAll GetOutput ]
  blobbyLogoImp SetMaximumDistance .2
  blobbyLogoImp SetSampleDimensions 64 64 64
  blobbyLogoImp SetAdjustDistance .5

# extract an iso surface
vtkContourFilter blobbyLogoIso
blobbyLogoIso SetInput [blobbyLogoImp GetOutput ]
  blobbyLogoIso SetValue 1  .1

# make normals
vtkPolyDataNormals blobbyLogoNormals
blobbyLogoNormals SetInput [blobbyLogoIso GetOutput ]
  blobbyLogoNormals SetFeatureAngle 60.0
  blobbyLogoNormals SetMaxRecursionDepth 100

# map to rendering primitives
vtkPolyDataMapper blobbyLogoMapper
blobbyLogoMapper SetInput [blobbyLogoNormals GetOutput ]
  blobbyLogoMapper ScalarVisibilityOff

# now an actor
vtkActor blobbyLogo
  blobbyLogo SetMapper blobbyLogoMapper
  blobbyLogo SetProperty [eval dented { 2 }]
eval [blobbyLogo GetProperty] SetDiffuseColor $banana
[blobbyLogo GetProperty] SetOpacity .5

# position the letters
VTransform Identity
VTransform Translate  -.5 0 .7
VTransform RotateY  50

KTransform Identity
KTransform Translate  .5 0 -.1
KTransform RotateY  -50

# move the polygonal letters to the front
 eval [logo GetProperty] SetDiffuseColor $tomato
logo SetPosition 0 0 0
  
vtkCamera aCam
  aCam SetFocalPoint  0.340664 0.470782 0.34374
  aCam SetPosition  0.698674 1.45247 2.89482
  aCam SetViewAngle  30
  aCam SetViewUp  0 1 0


#  now  make a renderer and tell it about lights and actors
renWin SetSize  640  480
  
ren1 SetActiveCamera  aCam
ren1 AddActor logo
ren1 AddActor blobbyLogo

ren1 SetBackground 1 1 1

renWin Render 

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

source prman.tcl

set RIBPrefix vtklogo
puts "Hitting the p key in the render window will create a RIB file"
