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
        floorMapper SetLookupTable lut
vtkActor floorActor
	floorActor SetMapper floorMapper
	[floorActor GetProperty] SetRepresentationToWireframe
	[floorActor GetProperty] SetColor 0 0 0

#vtkStructuredGridGeometryFilter subFloorComp
#	subFloorComp SetExtent 0 37 0 15 22 22
#	subFloorComp SetInput [pl3d GetOutput]
#vtkPolyDataMapper subFloorMapper
#	subFloorMapper SetInput [subFloorComp GetOutput]
#        subFloorMapper SetLookupTable lut
#	eval subFloorMapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
#vtkActor subFloorActor
#	subFloorActor SetMapper subFloorMapper

#vtkStructuredGridGeometryFilter subFloor2Comp
#	subFloor2Comp SetExtent 0 37 60 75 22 22
#	subFloor2Comp SetInput [pl3d GetOutput]
#vtkPolyDataMapper subFloor2Mapper
#	subFloor2Mapper SetInput [subFloor2Comp GetOutput]
#        subFloor2Mapper SetLookupTable lut
#	eval subFloor2Mapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
#vtkActor subFloor2Actor
#	subFloor2Actor SetMapper subFloor2Mapper

vtkStructuredGridGeometryFilter postComp
	postComp SetExtent 10 10 0 75 0 37
	postComp SetInput [pl3d GetOutput]
vtkPolyDataMapper postMapper
	postMapper SetInput [postComp GetOutput]
        postMapper SetLookupTable lut
	eval postMapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor postActor
	postActor SetMapper postMapper
	[postActor GetProperty] SetColor 0 0 0

#vtkStructuredGridGeometryFilter fanComp
#	fanComp SetExtent 0 37 38 38 0 37
#	fanComp SetInput [pl3d GetOutput]
#vtkPolyDataMapper fanMapper
#	fanMapper SetInput [fanComp GetOutput]
#        fanMapper SetLookupTable lut
#	eval fanMapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
#vtkActor fanActor
#	fanActor SetMapper fanMapper
#	[fanActor GetProperty] SetColor 0 0 0

# streamers
#
# spherical seed points
vtkPointSource rake
    rake SetCenter -0.74 0 0.3
    rake SetNumberOfPoints 2
# a line of seed points
vtkStructuredGridGeometryFilter seedsComp
    seedsComp SetExtent 10 10 38 38 17 17
    seedsComp SetInput [pl3d GetOutput]
vtkStreamLine streamers
    streamers VorticityOn
    streamers SetInput [pl3d GetOutput]
#    streamers SetSource [rake GetOutput]
    streamers SetSource [seedsComp GetOutput]
    streamers SetMaximumPropagationTime 250
    streamers OrientationScalarsOn
    streamers SetIntegrationStepLength .2
    streamers SetStepLength .25
    streamers SetNumberOfThreads 1

#vtkPolyLineRangeSelector selector
#    selector SetInput [streamers GetOutput]
#    selector SetStartingLineNumber 0
#	selector SetEndingLineNumber 0
#	selector SetStartingSegmentNumber 0
#	selector SetEndingSegmentNumber 200

vtkTubeFilter tubes
    #tubes SetInput [selector GetOutput]
    tubes SetInput [streamers GetOutput]
    tubes SetNumberOfSides 8
    tubes SetRadius .08
    tubes SetVaryRadius 0
vtkPolyDataMapper mapTubes
    mapTubes SetInput [tubes GetOutput]
streamers Update
    eval mapTubes SetScalarRange [[streamers GetOutput] GetScalarRange]
vtkActor tubesActor
    tubesActor SetMapper mapTubes

# outline
vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    set outlineProp [outlineActor GetProperty]
    eval $outlineProp SetColor 0 0 0

# Create graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor floorActor
#ren1 AddActor subFloorActor
#ren1 AddActor subFloor2Actor
ren1 AddActor postActor
#ren1 AddActor fanActor
ren1 AddActor tubesActor

vtkCamera aCam
    aCam SetFocalPoint 0.872 -1.20 1.57
    aCam SetPosition 1.16 -5.99 1.57
    aCam SetViewUp -0.050 -0.0036 0.999
    aCam SetClippingRange .458 45.8

ren1 SetBackground .1 .2 .4
ren1 SetActiveCamera aCam
renWin SetSize 400 400

iren Initialize
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render
#renWin SetFileName "LOx.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .






