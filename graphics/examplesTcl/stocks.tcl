catch {load vtktcl}
# this is a tcl script for the stock case study
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin


#create the outline
vtkAppendPolyData apf
vtkOutlineFilter olf
olf SetInput [apf GetOutput]
vtkPolyDataMapper outlineMapper
outlineMapper SetInput [olf GetOutput]
vtkActor outlineActor
outlineActor SetMapper outlineMapper
set zpos 0

# create the stocks
proc AddStock {prefix name x y z} {
    global zpos

    # create labels
    vtkTextSource $prefix.TextSrc
    $prefix.TextSrc SetText "$name"
    $prefix.TextSrc SetBacking 0
    vtkPolyDataMapper $prefix.LabelMapper
    $prefix.LabelMapper SetInput [$prefix.TextSrc GetOutput]
    vtkFollower $prefix.LabelActor
    $prefix.LabelActor SetMapper $prefix.LabelMapper
    $prefix.LabelActor SetPosition $x $y $z
    $prefix.LabelActor SetScale 0.25 0.25 0.25
    eval $prefix.LabelActor SetOrigin [$prefix.LabelMapper GetCenter]

    # create a sphere source and actor
    vtkPolyDataReader $prefix.PolyDataRead
    $prefix.PolyDataRead SetFileName "../../../vtkdata/$prefix.vtk"
    vtkTubeFilter $prefix.TubeFilter
    $prefix.TubeFilter SetInput [$prefix.PolyDataRead GetOutput]
    $prefix.TubeFilter SetNumberOfSides 8
    $prefix.TubeFilter SetRadius 0.5
    $prefix.TubeFilter SetRadiusFactor 10000
    
    vtkTransform $prefix.Transform
    $prefix.Transform Translate 0 0 $zpos
    $prefix.Transform Scale 0.15 1 1
    vtkTransformPolyDataFilter $prefix.TransformFilter
    $prefix.TransformFilter SetInput [$prefix.TubeFilter GetOutput]
    $prefix.TransformFilter SetTransform $prefix.Transform
    
    # increment zpos
    set zpos [expr $zpos + 10]

    vtkPolyDataMapper $prefix.StockMapper
    $prefix.StockMapper SetInput [$prefix.TransformFilter GetOutput]
    vtkActor $prefix.StockActor
    $prefix.StockActor SetMapper $prefix.StockMapper
    $prefix.StockMapper SetScalarRange 0 8000
    [$prefix.StockActor GetProperty] SetAmbient 0.5
    [$prefix.StockActor GetProperty] SetDiffuse 0.5

    apf AddInput [$prefix.TransformFilter GetOutput]
    ren1 AddActor $prefix.StockActor
    ren1 AddActor $prefix.LabelActor
    $prefix.LabelActor SetCamera [ren1 GetActiveCamera]
}

# set up the stocks
AddStock GE "GE" 94 46 4
AddStock GM "GM" 107 39 14 
AddStock IBM "IBM" 92 70 16 
AddStock DEC "DEC" 70 19 26 

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 SetBackground 0.1 0.2 0.4
#renWin SetSize 1200 950
renWin SetSize 1200 600

# render the image
#
renWin SetFileName "stocks2.ppm"
[ren1 GetActiveCamera] SetViewAngle 10
ren1 ResetCamera
#[ren1 GetActiveCamera] Zoom 1.9
[ren1 GetActiveCamera] Zoom 2.8
[ren1 GetActiveCamera] Elevation 90
[ren1 GetActiveCamera] SetViewUp 0 0 -1
iren Initialize

#renWin SetFileName stocks.tcl.ppm
#renWin SaveImageAsPPM

puts "done"

# prevent the tk window from showing up then start the event loop
wm withdraw .










