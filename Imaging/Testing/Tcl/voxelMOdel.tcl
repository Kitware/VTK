package require vtktcl_interactor


# get the interactor ui

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
  voxelModel SetInput [sphereModel GetOutput]
  voxelModel SetSampleDimensions 21 21 21

if {[catch {set channel [open voxelModel.vtk w]}] == 0 } {
   close $channel
   file delete -force test.tmp

   vtkDataSetWriter aWriter
     aWriter SetFileName voxelModel.vtk
     aWriter SetInput [voxelModel GetOutput]
     aWriter Update

   vtkDataSetReader aReader
     aReader SetFileName voxelModel.vtk

   vtkContourFilter voxelSurface
     voxelSurface SetInput [aReader GetOutput]
     voxelSurface SetValue 0 .999

   vtkPolyDataMapper voxelMapper
     voxelMapper SetInput [voxelSurface GetOutput]

   vtkActor voxelActor
     voxelActor SetMapper voxelMapper

   vtkPolyDataMapper sphereMapper
     sphereMapper SetInput [sphereModel GetOutput]

   vtkActor sphereActor
     sphereActor SetMapper sphereMapper

   ren1 AddActor sphereActor
   ren1 AddActor voxelActor

   ren1 SetBackground .1 .2 .4
   renWin SetSize 256 256
   [ren1 GetActiveCamera] SetViewUp 0 -1 0;
   [ren1 GetActiveCamera] Azimuth 180
   [ren1 GetActiveCamera] Dolly 1.75
   ren1 ResetCameraClippingRange

   iren Initialize;

# render the image
#
   iren SetUserMethod {wm deiconify .vtkInteract};


# prevent the tk window from showing up then start the event loop
   wm withdraw .
   file delete -force voxelModel.vtk
}


