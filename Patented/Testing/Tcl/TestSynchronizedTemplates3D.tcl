package require vtk
package require vtkinteraction

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetDataSpacing 3.2 3.2 1.5
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

# write isosurface to file
#vtkSynchronizedTemplates3D stemp
vtkKitwareContourFilter stemp
    stemp SetInput [reader GetOutput]
    stemp SetValue 0 1150
    stemp Update


vtkPolyDataMapper mapper
    mapper SetInput [stemp GetOutput]
	mapper ScalarVisibilityOff
    
vtkActor head
    head SetMapper mapper
    eval [head GetProperty] SetColor 1 0.7 0.6

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
renWin SetSize 400 400
eval ren1 SetBackground 0.5 0.5 0.6

[ren1 GetActiveCamera] SetPosition 99.8847 537.926 15 
[ren1 GetActiveCamera] SetFocalPoint 99.8847 109.81 15 
[ren1 GetActiveCamera] SetViewAngle 20
[ren1 GetActiveCamera] SetViewUp 0 0 -1 

ren1 ResetCameraClippingRange

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

