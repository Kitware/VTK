catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

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

vtkContourFilter voxelSurface
  voxelSurface SetInput [voxelModel GetOutput]
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
iren Initialize;

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract};

renWin SetFileName "voxelModel.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


