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
    vtkVectorText $prefix.TextSrc
    $prefix.TextSrc SetText "$name"
    vtkPolyDataMapper $prefix.LabelMapper
    $prefix.LabelMapper SetInput [$prefix.TextSrc GetOutput]
    vtkFollower $prefix.LabelActor
    $prefix.LabelActor SetMapper $prefix.LabelMapper
    $prefix.LabelActor SetPosition $x $y $z
    $prefix.LabelActor SetScale 2 2 2
    eval $prefix.LabelActor SetOrigin [$prefix.LabelMapper GetCenter]

    # create a sphere source and actor
    vtkPolyDataReader $prefix.PolyDataRead
    $prefix.PolyDataRead SetFileName "../../../vtkdata/$prefix.vtk"
    vtkRibbonFilter $prefix.RibbonFilter;
    $prefix.RibbonFilter SetInput [$prefix.PolyDataRead GetOutput];
    $prefix.RibbonFilter VaryWidthOn;
    $prefix.RibbonFilter SetWidthFactor 5;
    $prefix.RibbonFilter SetDefaultNormal 0 1 0;
    $prefix.RibbonFilter UseDefaultNormalOn;
    
    vtkLinearExtrusionFilter $prefix.Extrude;
    $prefix.Extrude SetInput [$prefix.RibbonFilter GetOutput];
    $prefix.Extrude SetVector 0 1 0;
    $prefix.Extrude SetExtrusionType 1;
    $prefix.Extrude SetScaleFactor 0.7;

#    vtkTubeFilter $prefix.TubeFilter
#    $prefix.TubeFilter SetInput [$prefix.PolyDataRead GetOutput]
#    $prefix.TubeFilter SetNumberOfSides 8
#    $prefix.TubeFilter SetRadius 0.5
#    $prefix.TubeFilter SetRadiusFactor 5
#    $prefix.TubeFilter SetRadiusFactor 10000
#    $prefix.TubeFilter SetVaryRadiusToVaryRadiusByScalar
    
 
    vtkTransform $prefix.Transform
    $prefix.Transform Translate 0 0 $zpos
    $prefix.Transform Scale 0.15 1 1
    vtkTransformPolyDataFilter $prefix.TransformFilter
#    $prefix.TransformFilter SetInput [$prefix.TubeFilter GetOutput]
    $prefix.TransformFilter SetInput [$prefix.Extrude GetOutput]
    $prefix.TransformFilter SetTransform $prefix.Transform
    
    # increment zpos
    set zpos [expr $zpos + 10]

    vtkPolyDataMapper $prefix.StockMapper
    $prefix.StockMapper SetInput [$prefix.TransformFilter GetOutput]
    vtkActor $prefix.StockActor
    $prefix.StockActor SetMapper $prefix.StockMapper
    $prefix.StockMapper SetScalarRange 0 8000
#    [$prefix.StockActor GetProperty] SetAmbient 0.5
#    [$prefix.StockActor GetProperty] SetDiffuse 0.5

    apf AddInput [$prefix.TransformFilter GetOutput]
    ren1 AddActor $prefix.StockActor
    ren1 AddActor $prefix.LabelActor
    $prefix.LabelActor SetCamera [ren1 GetActiveCamera]
}

# set up the stocks
AddStock GE "GE" 104 55 3
AddStock GM "GM" 92 39 13 
AddStock IBM "IBM" 96 80 17 
AddStock DEC "DEC" 56 25 27 

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 600 460
#renWin SetSize 1200 600

# render the image
#
[ren1 GetActiveCamera] SetViewAngle 10
ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 1.9
#[ren1 GetActiveCamera] Zoom 2.8
#[ren1 GetActiveCamera] Elevation 90
#[ren1 GetActiveCamera] SetViewUp 0 0 -1
iren Initialize

renWin SetFileName stocks.tcl.ppm
renWin SaveImageAsPPM

puts "done"

# prevent the tk window from showing up then start the event loop
wm withdraw .










