catch {load vtktcl}
# Generate marching cubes head model (full resolution)

# get the interactor ui and colors
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# create pipeline
# reader reads slices
vtkVolume16Reader v16
    v16 SetDataDimensions 256 256
    v16 SetDataByteOrderToLittleEndian
    v16 SetFilePrefix "../../../vtkdata/fullHead/headsq"
    v16 SetDataSpacing 0.8 0.8 1.5
    v16 SetImageRange 1 94;#uncomment for the whole head
    v16 SetDataMask 0x7fff

# write isosurface to file
vtkSliceCubes mcubes
    mcubes SetReader v16
    mcubes SetValue 1150
    mcubes SetFileName "fullHead.tri"
    mcubes SetLimitsFileName "fullHead.lim"
    mcubes Update

# read from file
vtkMCubesReader reader
reader SetFileName "fullHead.tri"
reader SetLimitsFileName "fullHead.lim"

vtkPolyDataMapper mapper
    mapper SetInput [reader GetOutput]
    
vtkActor head
    head SetMapper mapper
    eval [head GetProperty] SetColor $raw_sienna

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor head
ren1 SetBackground 1 1 1
renWin SetSize 500 500
eval ren1 SetBackground $slate_grey
[ren1 GetActiveCamera] Zoom 1.5
[ren1 GetActiveCamera] Azimuth 180
[ren1 GetActiveCamera] Elevation -90

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

iren Initialize

#renWin SetFileName "genHead.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

