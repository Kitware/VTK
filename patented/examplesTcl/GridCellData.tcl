catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Generate marching cubes head model (full resolution)

# get the interactor ui and colors
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

vtkMultiProcessController c

vtkPoints pts
pts SetNumberOfPoints 27
pts SetPoint 0 0 1 0
pts SetPoint 1 0 1 1
pts SetPoint 2 0 1 2
pts SetPoint 3 0 2 0
pts SetPoint 4 0 2 1
pts SetPoint 5 0 2 2
pts SetPoint 6 0 3 0
pts SetPoint 7 0 3 1
pts SetPoint 8 0 3 2

pts SetPoint 9 1 0.5 0
pts SetPoint 10 1 0.5 1
pts SetPoint 11 1 0.5 2
pts SetPoint 12 1 2 0.5
pts SetPoint 13 1 2 1.5
pts SetPoint 14 1 2 2.5
pts SetPoint 15 1 3.5 1
pts SetPoint 16 1 3.5 2
pts SetPoint 17 1 3.5 3

pts SetPoint 18 2 0 0
pts SetPoint 19 2 0 1
pts SetPoint 20 2 0 2
pts SetPoint 21 2 2 0
pts SetPoint 22 2 2 1
pts SetPoint 23 2 2 2
pts SetPoint 24 2 4 0
pts SetPoint 25 2 4 1
pts SetPoint 26 2 4 2

vtkScalars ps
ps SetNumberOfScalars 27
ps SetScalar 0 0
ps SetScalar 1 0
ps SetScalar 2 0
ps SetScalar 3 0
ps SetScalar 4 1
ps SetScalar 5 0
ps SetScalar 6 0
ps SetScalar 7 0
ps SetScalar 8 0

ps SetScalar 9 0
ps SetScalar 10 1
ps SetScalar 11 0
ps SetScalar 12 1
ps SetScalar 13 0
ps SetScalar 14 1
ps SetScalar 15 0
ps SetScalar 16 1
ps SetScalar 17 0

ps SetScalar 18 0
ps SetScalar 19 0
ps SetScalar 20 0
ps SetScalar 21 0
ps SetScalar 22 1
ps SetScalar 23 0
ps SetScalar 24 0
ps SetScalar 25 0
ps SetScalar 26 0

vtkScalars cs
cs SetNumberOfScalars 8
cs SetScalar 0 0
cs SetScalar 1 1
cs SetScalar 2 2
cs SetScalar 3 3
cs SetScalar 4 4
cs SetScalar 5 5
cs SetScalar 6 6
cs SetScalar 7 7

vtkStructuredGrid gr
   gr SetDimensions 3 3 3
   gr SetPoints pts
   [gr GetCellData] SetScalars cs
   [gr GetPointData] SetScalars ps





# write isosurface to file
vtkGridSynchronizedTemplates3D stemp
#vtkKitwareContourFilter stemp
    stemp SetNumberOfThreads 2
    stemp SetInput gr
    stemp SetValue 0 0.5
    stemp ComputeScalarsOff
    stemp ComputeNormalsOff

vtkPolyDataMapper mapper
    mapper SetInput [stemp GetOutput]
    mapper ScalarVisibilityOn
    mapper SetScalarRange 0 7

vtkActor head
    head SetMapper mapper




vtkExtractEdges edges
  edges SetInput gr

vtkTubeFilter tubes
  tubes SetInput [edges GetOutput]
  tubes SetRadius 0.015
  tubes SetNumberOfSides 8

vtkPolyDataMapper mapper2
    mapper2 SetInput [tubes GetOutput]
    mapper2 ScalarVisibilityOff

vtkActor actor2
    actor2 SetMapper mapper2
    [actor2 GetProperty] SetAmbient 0.5

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
ren1 AddActor actor2
ren1 SetBackground 0.8 0.8 1.0
renWin SetSize 500 500
#eval ren1 SetBackground $slate_grey

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}


set cam [ren1 GetActiveCamera]
$cam SetFocalPoint  1.4 2.2 1.3
$cam SetPosition -5  16  2.5
$cam SetViewUp 0.717597 0.307331 -0.624982
ren1 ResetCameraClippingRange
$cam Zoom 1.8
renWin Render

#renWin SetFileName "genHead.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

