catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl
source colors.tcl

# create pipeline
#
vtkVolume16Reader v16
    v16 SetDataDimensions 128 128 
    [v16 GetOutput] SetOrigin 0.0 0.0 0.0
    v16 SetFileTypeLittleEndian
    v16 SetFilePrefix "../../../data/headsq/half"
    v16 SetImageRange 45 45
    v16 SetDataSpacing 1.6 1.6 1.5
vtkContourFilter iso
    iso SetInput [v16 GetOutput]
    iso GenerateValues 6 600 1200
vtkStripper stripper
    stripper SetInput [iso GetOutput]
vtkTubeFilter tuber
    tuber SetInput [stripper GetOutput]
    tuber SetNumberOfSides 4
vtkPolyMapper isoMapper
    isoMapper SetInput [tuber GetOutput]
    isoMapper SetScalarRange 600 1200
vtkActor isoActor
    isoActor SetMapper isoMapper

vtkOutlineFilter outline
    outline SetInput [v16 GetOutput]
vtkPolyMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
#eval $outlineProp SetColor 0 0 0

# The graphics stuff
#
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# Add the actors to the renderer, set the background and size
#
$ren1 AddActor outlineActor
$ren1 AddActor isoActor
$ren1 SetBackground 1 1 1
$renWin SetSize 400 400
[$ren1 GetActiveCamera] Zoom 1.4
$ren1 SetBackground 0.1 0.2 0.4

$iren Initialize

$renWin SetFileName "lineStrip.tcl.ppm"
#$renWin SaveImageAsPPM

$iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

