catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl


source ../../imaging/examplesTcl/vtkImageInclude.tcl
source ../../imaging/examplesTcl/TkImageViewerInteractor.tcl



vtkConeSource cone
  cone SetHeight 1.5
vtkPolyDataMapper coneMapper
  coneMapper SetInput [cone GetOutput]
vtkActor coneActor
  coneActor SetMapper coneMapper
  [coneActor GetProperty] SetColor 0.8 0.9 1.0


vtkSphereSource sphere
vtkPolyDataMapper sphereMapper
  sphereMapper SetInput [sphere GetOutput]
vtkActor sphereActor
  sphereActor SetMapper sphereMapper
  [sphereActor GetProperty] SetColor 0.7 1.0 0.7



vtkRenderer ren1
    ren1 SetBackground  0.8 0.4 0.3
    # WinNT mixes both background colors in a stripped pattern
    #ren1 SetBackground  0.1 0.2 0.4
    ren1 AddActor sphereActor
vtkRenderer ren2
    ren2 SetBackground 0.8 0.4 0.3
    ren2 AddActor coneActor
    ren2 SetActiveCamera [ren1 GetActiveCamera]
vtkRenderWindow renWin1
    renWin1 AddRenderer ren1
    renWin1 SetPosition 10 10
    renWin1 SetSize 256 256
vtkRenderWindow renWin2
    renWin2 AddRenderer ren2
    renWin2 SetPosition 275 10
    renWin2 SetSize 256 256
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin1


renWin1 Render
renWin2 Render

vtkRendererSource ren1Image
  ren1Image SetInput ren1
  ren1Image DepthValuesOn
  ren1Image Update

vtkRendererSource ren2Image
  ren2Image SetInput ren2
  ren2Image DepthValuesOn
  ren2Image Update

vtkImageComposite composite
  composite AddInput [ren1Image GetOutput]
  composite AddInput [ren2Image GetOutput]


# through up the zbuffer
vtkFieldDataToAttributeDataFilter zScalars
zScalars SetInput [composite GetOutput]
#zScalars SetInput [ren2Image GetOutput]
zScalars SetInputFieldToPointDataField
zScalars SetOutputAttributeDataToPointData
zScalars SetScalarComponent 0 ZBuffer 0






#tk_messageBox -message [[[[zScalars GetOutput] GetPointData] GetScalars] Print]

vtkImageViewer viewer
#viewer SetColorLevel 0.5
#viewer SetColorWindow 1.0
#viewer SetInput [zScalars GetOutput]
viewer SetColorLevel 127.5
viewer SetColorWindow 255

viewer SetInput [composite GetOutput]
[viewer GetImageWindow] DoubleBufferOn



# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
toplevel .top 

frame .top.f1 

vtkTkImageViewerWidget  .top.f1.r1 -width 256 -height 256 -iv viewer
#    BindTkRenderWidget .top.f1.r1

button .top.btn  -text Quit -command exit

pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x

BindTkImageViewer .top.f1.r1 




# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}


# prevent the tk window from showing up then start the event loop
wm withdraw .





