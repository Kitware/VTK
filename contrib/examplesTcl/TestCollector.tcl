catch {load vtktcl}
# Generate marching cubes head model (full resolution)

# get the interactor ui and colors
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# create pipeline
# reader reads slices
vtkImageReader v16
    v16 SetDataExtent 0 255 0 255 30 50
    v16 SetDataByteOrderToLittleEndian
    v16 SetFilePrefix "../../../vtkdata/fullHead/headsq"
    v16 SetDataSpacing 0.8 0.8 1.5
    v16 SetDataMask 0x7fff
    v16 SetEndMethod {puts "Read: [[v16 GetOutput] GetUpdateExtent]"}

# write isosurface to file
#vtkSynchronizedTemplates3D stemp
vtkSynchronizedTemplates3D stemp
    stemp SetInput [v16 GetOutput]
    stemp SetValue 0 1150

vtkDecimatePro decimate
   decimate SetInput [stemp GetOutput]
   decimate SetTargetReduction 0.5

vtkPolyDataCollector collector
   collector SetInput [decimate GetOutput]
   collector SetInputMemoryLimit 1000

vtkPolyDataMapper mapper
    mapper SetInput [collector GetOutput]
	mapper ScalarVisibilityOff
    
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
[ren1 GetActiveCamera] Elevation 90

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

iren Initialize

#renWin SetFileName "genHead.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

