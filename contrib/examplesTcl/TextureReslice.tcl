# A very basic medical image volume visualization tool

# When you run this example, type 'o' and 'c' to switch
# between object and camera interaction,  otherwise you
# will miss the full effect


catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }



# read data file
# set the originA so that (0.0,0.0,0.0) is the center of the image

# some Tcl-induced shenanigans...
array set spacingA {0 1.0 1 1.0 2 2.0}
array set extentA  {0 0 1 255 2 0 3 255 4 1 5 93}
array set originA  {0 -127.5 1 -127.5 2 -94.0}

vtkImageReader reader
  reader ReleaseDataFlagOff
  reader SetDataByteOrderToLittleEndian
  reader SetDataSpacing $spacingA(0) $spacingA(1) $spacingA(2)
  reader SetDataExtent $extentA(0) $extentA(1) \
          $extentA(2) $extentA(3) $extentA(4) $extentA(5)
  reader SetDataOrigin $originA(0) $originA(1) $originA(2)
  reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader SetDataMask 0x7fff
  reader UpdateWholeExtent

# transform shared by reslice filter and texture mapped plane actor
vtkMatrix4x4 matrix
vtkMatrixToLinearTransform transform
  transform SetInput matrix

# slice extraction filter
vtkImageReslice reslice
  reslice SetInput [reader GetOutput]
  reslice SetResliceTransform transform
  reslice InterpolateOn
  reslice SetBackgroundLevel 1023
  reslice SetOutputSpacing $spacingA(0) $spacingA(1) $spacingA(2)
  reslice SetOutputOrigin $originA(0) $originA(1) 0.0
  reslice SetOutputExtent $extentA(0) $extentA(1) $extentA(2) $extentA(3) 0 0

# lookup table for texture map
vtkLookupTable table
  table SetTableRange 100 2000
  table SetSaturationRange 0 0
  table SetHueRange 0 0
  table SetValueRange 0 1
  table Build

# texture from reslice filter
vtkTexture atext
  atext SetInput [reslice GetOutput]
  atext SetLookupTable table
  atext InterpolateOn

# need a plane to texture map onto
vtkPlaneSource plane
  plane SetXResolution 1
  plane SetYResolution 1
  plane SetOrigin [expr $originA(0) + $spacingA(0) * $extentA(0) - 0.5] \
          [expr $originA(1) + $spacingA(1) *$extentA(2) - 0.5] \
          0.0
  plane SetPoint1 [expr $originA(0) + $spacingA(0) * $extentA(1) + 0.5] \
          [expr $originA(1) + $spacingA(1) * $extentA(2) - 0.5] \
          0.0
  plane SetPoint2 [expr $originA(0) + $spacingA(0) * $extentA(0) - 0.5] \
          [expr $originA(1) + $spacingA(1) * $extentA(3) + 0.5] \
          0.0

# generate texture coordinates
vtkTextureMapToPlane tmapper
  tmapper SetInput [plane GetOutput]

# mapper for the textured plane
vtkDataSetMapper mapper
  mapper SetInput [tmapper GetOutput]

# put everything together (note that the same transform
# is used for slice extraction and actor positioning)
vtkActor actor
  actor SetMapper mapper
  actor SetTexture atext
  actor SetUserMatrix matrix
  actor SetOrigin 0.0 0.0 0.0

# create rendering stuff
vtkRenderer ren1

vtkRenderWindow renWin
  renWin AddRenderer ren1

vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# add a frame around the volume
vtkOutlineFilter outline
  outline SetInput [reader GetOutput]

vtkPolyDataMapper outlineMapper
  outlineMapper SetInput [outline GetOutput]

vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  [outlineActor GetProperty] SetColor 1.0000 0.8431 0.0000

# add the actors to the renderer, set the background and size
ren1 AddActor actor
ren1 AddActor outlineActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500

# apply transformations
vtkTransform tmpTrans
tmpTrans RotateX 10.0
tmpTrans RotateY 10.0
tmpTrans GetMatrix matrix

# don't show the tcl window
wm withdraw .

# render the image
renWin Render


renWin SetFileName "TextureReslice.tcl.ppm"
#renWin SaveImageAsPPM
