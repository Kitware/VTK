# A very basic medical image volume visualization tool

# When you run this example, type 'o' and 'c' to switch
# between object and camera interaction,  otherwise you
# will miss the full effect


catch {load vtktcl}


# read data file
# set the origin so that (0.0,0.0,0.0) is the center of the image

# some Tcl-induced shenanigans...
array set spacing {0 1.0 1 1.0 2 2.0}
array set extent  {0 0 1 255 2 0 3 255 4 1 5 93}
array set origin  {0 -127.5 1 -127.5 2 -94.0}

vtkImageReader reader
  reader ReleaseDataFlagOff
  reader SetDataByteOrderToLittleEndian
  reader SetDataSpacing $spacing(0) $spacing(1) $spacing(2)
  reader SetDataExtent $extent(0) $extent(1) \
          $extent(2) $extent(3) $extent(4) $extent(5)
  reader SetDataOrigin $origin(0) $origin(1) $origin(2)
  reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
  reader SetDataMask 0x7fff
  reader UpdateWholeExtent

# transform shared by reslice filter and texture mapped plane actor
vtkTransform transform

# slice extraction filter
vtkImageReslice reslice
  reslice SetInput [reader GetOutput]
  reslice SetResliceTransform transform
  reslice InterpolateOn
  reslice SetBackgroundLevel 1023
  reslice SetOutputSpacing $spacing(0) $spacing(1) $spacing(2)
  reslice SetOutputOrigin $origin(0) $origin(1) 0.0
  reslice SetOutputExtent $extent(0) $extent(1) $extent(2) $extent(3) 0 0

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
  plane SetOrigin [expr $origin(0) + $spacing(0) * $extent(0) - 0.5] \
          [expr $origin(1) + $spacing(1) *$extent(2) - 0.5] \
          0.0
  plane SetPoint1 [expr $origin(0) + $spacing(0) * $extent(1) + 0.5] \
          [expr $origin(1) + $spacing(1) * $extent(2) - 0.5] \
          0.0
  plane SetPoint2 [expr $origin(0) + $spacing(0) * $extent(0) - 0.5] \
          [expr $origin(1) + $spacing(1) * $extent(3) + 0.5] \
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
  actor SetUserMatrix [transform GetMatrix]
  actor SetOrigin 0.0 0.0 0.0

# create rendering stuff
vtkRenderer ren1

vtkRenderWindow renWin
  renWin AddRenderer ren1

vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# add a frame around the volume
vtkOutlineFilter frame
  frame SetInput [reader GetOutput]

vtkPolyDataMapper frameMapper
  frameMapper SetInput [frame GetOutput]

vtkActor frameActor
  frameActor SetMapper frameMapper
  [frameActor GetProperty] SetColor 1.0000 0.8431 0.0000

# add the actors to the renderer, set the background and size
ren1 AddActor actor
ren1 AddActor frameActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500

# apply transformations
transform RotateX 10.0
transform RotateY 10.0

# don't show the tcl window
wm withdraw .

# render the image
renWin Render


renWin SetFileName "TextureReslice.tcl.ppm"
#renWin SaveImageAsPPM
