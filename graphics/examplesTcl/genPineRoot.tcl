catch {load vtktcl}
# Generate marching cubes pine root model (256^3 model)

# get the interactor ui and colors
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkVolume16Reader v16
    v16 SetDataDimensions 256 256
    v16 SetDataByteOrderToBigEndian
    v16 SetFilePrefix "../../../vtkdata/pineRoot/pine_root"
    v16 SetImageRange 50 100
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

vtkPolyDataMapper mapper
    mapper SetInput [reader GetOutput]
    
vtkActor a
    a SetMapper mapper
    eval [a GetProperty] SetColor $raw_sienna

# Add the actors to the renderer, set the background and size
#
ren1 AddActor a
ren1 SetBackground 1 1 1
renWin SetSize 500 500
eval ren1 SetBackground $slate_grey

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName "valid/genPineRoot.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
