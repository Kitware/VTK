catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

## LOx post CFD case study

# get helper scripts
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# read data
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA/postxyz.bin"
    pl3d SetQFileName "$VTK_DATA/postq.bin"
    pl3d SetScalarFunctionNumber 153
    pl3d SetVectorFunctionNumber 200
    pl3d Update

#blue to red lut
#
vtkLookupTable lut
    lut SetHueRange 0.667 0.0

# computational planes
vtkStructuredGridGeometryFilter floorComp
	floorComp SetExtent 0 37 0 75 0 0
	floorComp SetInput [pl3d GetOutput]
	floorComp Update
vtkPolyDataMapper floorMapper
	floorMapper SetInput [floorComp GetOutput]
        floorMapper ScalarVisibilityOff
vtkActor floorActor
	floorActor SetMapper floorMapper
	[floorActor GetProperty] SetColor .9 .9 .9

vtkStructuredGridGeometryFilter postComp
	postComp SetExtent 10 10 0 75 0 37
	postComp SetInput [pl3d GetOutput]
vtkPolyDataMapper postMapper
	postMapper SetInput [postComp GetOutput]
        postMapper ScalarVisibilityOff
vtkActor postActor
	postActor SetMapper postMapper
	[postActor GetProperty] SetColor .9 .9 .9

# streamsurface
vtkLineSource rake
  rake SetResolution 40
  rake SetPoint1  -5 -1 1.3
  rake SetPoint2  -5  1 1.3
vtkStreamLine streamers
  streamers SetInput [pl3d GetOutput]
  streamers SetSource [rake GetOutput]
  streamers SetMaximumPropagationTime 250
  streamers SetStepLength [expr [[pl3d GetOutput] GetLength] / 1000.0]

vtkRuledSurfaceFilter scalarSurface
  scalarSurface SetInput [streamers GetOutput]
  scalarSurface SetOffset 0
  scalarSurface SetOnRatio 2
  scalarSurface PassLinesOn
  scalarSurface SetRuledModeToResample
  scalarSurface SetResolution 100 1
  scalarSurface SetDistanceFactor 30
vtkPolyDataMapper scalarSurfaceMapper
  scalarSurfaceMapper SetInput [scalarSurface GetOutput]
  eval scalarSurfaceMapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
  scalarSurfaceMapper SetLookupTable lut
  scalarSurfaceMapper SetResolveCoincidentTopologyToPolygonOffset
vtkActor scalarSurfaceActor
  scalarSurfaceActor SetMapper scalarSurfaceMapper
  [scalarSurfaceActor GetProperty] SetColor .9 .9 .9

vtkRuledSurfaceFilter stripedSurface
  stripedSurface SetInput [streamers GetOutput]
  stripedSurface SetOffset 1
  stripedSurface SetOnRatio 2
  stripedSurface SetRuledModeToResample
  stripedSurface SetResolution 100 1
  stripedSurface SetDistanceFactor 10
vtkPolyDataMapper stripedSurfaceMapper
  stripedSurfaceMapper SetInput [stripedSurface GetOutput]
  eval stripedSurfaceMapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
  stripedSurfaceMapper ScalarVisibilityOff
vtkActor stripedSurfaceActor
  stripedSurfaceActor SetMapper stripedSurfaceMapper
  [stripedSurfaceActor GetProperty] SetColor 0 0 1

# Create graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor floorActor
ren1 AddActor postActor
ren1 AddActor scalarSurfaceActor
ren1 AddActor stripedSurfaceActor

vtkCamera aCam
  aCam SetFocalPoint 0.283834 -0.677403 3.68526
  aCam SetPosition -0.934847 -4.27021 9.21436
  aCam SetViewUp 0.164623 0.810109 0.562693
  aCam SetViewAngle 30
  aCam SetClippingRange 0.36 36

ren1 SetBackground 1 1 1
ren1 SetActiveCamera aCam
renWin SetSize 400 400

iren Initialize
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .






