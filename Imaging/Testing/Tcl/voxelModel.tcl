package require vtk
package require vtkinteraction


# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

vtkSphereSource sphereModel
  sphereModel SetThetaResolution 10
  sphereModel SetPhiResolution 10

vtkVoxelModeller voxelModel
  voxelModel SetInputConnection [sphereModel GetOutputPort]
  voxelModel SetSampleDimensions 21 21 21
  voxelModel SetModelBounds -1.5 1.5 -1.5 1.5 -1.5 1.5
  voxelModel SetScalarTypeToBit
  voxelModel SetForegroundValue 1
  voxelModel SetBackgroundValue 0
if {[catch {set channel [open "voxelModel.vtk" "w"]}] == 0 } {
   close $channel
   file delete -force "voxelModel.vtk"

   vtkDataSetWriter aWriter
     aWriter SetFileName "voxelModel.vtk"
     aWriter SetInputConnection [voxelModel GetOutputPort]
     aWriter Update

   vtkDataSetReader aReader
     aReader SetFileName "voxelModel.vtk"

   vtkContourFilter voxelSurface
     voxelSurface SetInputConnection [aReader GetOutputPort]
     voxelSurface SetValue 0 .999

   vtkPolyDataMapper voxelMapper
     voxelMapper SetInputConnection [voxelSurface GetOutputPort]

   vtkActor voxelActor
     voxelActor SetMapper voxelMapper

   vtkPolyDataMapper sphereMapper
     sphereMapper SetInputConnection [sphereModel GetOutputPort]

   vtkActor sphereActor
     sphereActor SetMapper sphereMapper

   ren1 AddActor sphereActor
   ren1 AddActor voxelActor

   ren1 SetBackground .1 .2 .4
   renWin SetSize 256 256
   ren1 ResetCamera
   [ren1 GetActiveCamera] SetViewUp 0 -1 0;
   [ren1 GetActiveCamera] Azimuth 180
   [ren1 GetActiveCamera] Dolly 1.75
   ren1 ResetCameraClippingRange

   iren Initialize;

# render the image
#
   iren AddObserver UserEvent {wm deiconify .vtkInteract};


# prevent the tk window from showing up then start the event loop
   wm withdraw .

}


