catch {load vtktcl}
# Generate marching cubes pine root model (256^3 model)

# get the interactor ui and colors
source vtkInt.tcl
source colors.tcl

# First create the render master
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# create pipeline
#
vtkVolume16Reader v16
    v16 SetDataDimensions 256 256
    v16 SetFileTypeBigEndian
    v16 SetFilePrefix "../../../data/pineRoot/pine_root"
    v16 SetImageRange 1 256
    v16 SetDataSpacing 0.3125 0.3125 0.390625
    v16 SetDataMask 0x7fff

vtkSliceCubes mcubes
    mcubes SetReader v16
    mcubes SetValue 1750
    mcubes SetFileName "pine_root.tri"
    mcubes SetLimitsFileName "pine_root.lim"
    mcubes Update

vtkMCubesReader reader
    reader SetFileName "pine_root.tri"
    reader SetLimitsFileName "pine_root.lim"

vtkPolyMapper mapper
    mapper SetInput [reader GetOutput]
    
vtkActor a
    a SetMapper mapper
    eval [a GetProperty] SetColor $raw_sienna

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors a
$ren1 SetBackground 1 1 1
$renWin SetSize 500 500
eval $ren1 SetBackground $slate_grey

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
$iren Initialize
