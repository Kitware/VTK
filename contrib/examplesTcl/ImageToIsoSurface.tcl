catch {load vtktcl}
# get the interactor ui
source ../../graphics/examplesTcl/vtkInt.tcl
source ../../graphics/examplesTcl/colors.tcl
source ../../imaging/examplesTcl/vtkImageInclude.tcl

# First create the render master
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]
vtkLight lgt

# create pipeline
#

vtkImageSeriesReader v16
    v16 SetDataDimensions 128 128 
    v16 SetDataOrigin 0.0 0.0 0.0
    v16 SetFileTypeLittleEndian
    v16 SetFilePrefix "../../../data/headsq/half"
    v16 SetImageRange 1 90
    v16 SetDataAspectRatio 1.6 1.6 1.5
    v16 SetOutputScalarType $VTK_SHORT
    v16 DebugOn

#set region [[v16 GetOutput] Update]
#$region SetExtent 64 65 116 117 0 2

vtkImageToIsoSurface iso
#vtkMarchingCubes iso
   iso ComputeNormalsOn
   iso ComputeGradientsOff
   iso ComputeScalarsOn
   iso SetInput [v16 GetOutput]
   # 1 MB
   iso SetInputMemoryLimit 1000 
   iso SetValue 0 1150
   #iso SetValue 1 500
   iso DebugOn



#iso Update
#exit

vtkPolyMapper isoMapper
    isoMapper SetInput [iso GetOutput]
    isoMapper SetScalarRange 100 1500
vtkActor isoActor
    isoActor SetMapper isoMapper

# Add the actors to the renderer, set the background and size
#
$ren1 AddActor isoActor
$ren1 SetBackground 1 1 1
$ren1 AddLight lgt
$renWin SetSize 500 500
$ren1 SetBackground 0.1 0.2 0.4
$renWin DoubleBufferOff

set cam1 [$ren1 GetActiveCamera]
$cam1 Elevation 90
$cam1 SetViewUp 0 0 -1
$cam1 Zoom 1.3
$cam1 Azimuth 180
eval lgt SetPosition [$cam1 GetPosition]
eval lgt SetFocalPoint [$cam1 GetFocalPoint]

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}

$renWin Render
#$renWin SetFileName "headBone.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


