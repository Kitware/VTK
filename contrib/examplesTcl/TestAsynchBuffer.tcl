# this example uses AsynchornousBuffer to allow interaction while
# an iso surface is being computed.  A tempory low resolution object
# is created as a stand in.


catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
vtkLight lgt

# create pipeline
#

vtkImageReader reader
    reader SetDataByteOrderToLittleEndian
    reader SetDataExtent 0 127 0 127 1 93
    reader SetFilePrefix "../../../vtkdata/headsq/half"
    reader SetDataSpacing 1.6 1.6 1.5

vtkImageMarchingCubes iso
    iso SetInput [reader GetOutput]
    iso SetValue 0 1150
    iso ComputeNormalsOn
    iso ComputeScalarsOff

vtkVectorNorm gradient
  gradient SetInput [iso GetOutput]





vtkSphereSource sphere
   sphere SetRadius 68
   sphere SetCenter 100 100 69

vtkAsynchronousBuffer buf
   buf SetInput [sphere GetOutput]
   buf Update
   buf SetInput [iso GetOutput]
   buf BlockingOff


vtkDataSetMapper isoMapper
    isoMapper SetInput [buf GetOutput]
    isoMapper ScalarVisibilityOn
    isoMapper SetScalarRange 0 1200
    isoMapper ImmediateModeRenderingOn

vtkActor isoActor
    isoActor SetMapper isoMapper
set isoProp [isoActor GetProperty]
eval $isoProp SetColor $antique_white

vtkOutlineFilter outline
    outline SetInput [reader GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
#eval $outlineProp SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoActor
ren1 SetBackground 1 1 1
ren1 AddLight lgt
renWin SetSize 500 500
ren1 SetBackground 0.1 0.2 0.4

set cam1 [ren1 GetActiveCamera]
$cam1 Elevation 90
$cam1 SetViewUp 0 0 -1
$cam1 Zoom 1.3
eval lgt SetPosition [$cam1 GetPosition]
eval lgt SetFocalPoint [$cam1 GetFocalPoint]

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render
iren Initialize
#renWin SetFileName "headBone.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .




