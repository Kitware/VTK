catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl
source "colors.tcl"

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "../../../data/combxyz.bin"
    pl3d SetQFileName "../../../data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
vtkStructuredGridGeometryFilter plane
    plane SetInput [pl3d GetOutput]
    plane SetExtent 10 10 1 100 1 100
vtkStructuredGridGeometryFilter plane2
    plane2 SetInput [pl3d GetOutput]
    plane2 SetExtent 30 30 1 100 1 100
vtkStructuredGridGeometryFilter plane3
    plane3 SetInput [pl3d GetOutput]
    plane3 SetExtent 45 45 1 100 1 100
vtkAppendPolyData appendF
    appendF AddInput [plane GetOutput]
    appendF AddInput [plane2 GetOutput]
    appendF AddInput [plane3 GetOutput]
vtkWarpVector warp
    warp SetInput [appendF GetOutput]
    warp SetScaleFactor 0.005
vtkCastToConcrete caster
    caster SetInput [warp GetOutput]
vtkPolyDataNormals normals
    normals SetInput [caster GetPolyDataOutput]
    normals SetFeatureAngle 60
vtkDataSetMapper planeMapper
    planeMapper SetInput [normals GetOutput]
    planeMapper SetScalarRange 0.197813 0.710419
    planeMapper ScalarVisibilityOff
vtkActor planeActor
    planeActor SetMapper planeMapper
    set planeProp [planeActor GetProperty]
    eval $planeProp SetColor $salmon

vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    set outlineProp [outlineActor GetProperty]
    eval $outlineProp SetColor $black

ren1 AddActor outlineActor
ren1 AddActor planeActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
iren Initialize
[ren1 GetActiveCamera] Zoom 1.4

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render
#renWin SetFileName "velProf.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



