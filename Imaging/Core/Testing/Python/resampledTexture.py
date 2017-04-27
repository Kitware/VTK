#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Demonstrate automatic resampling of textures (i.e., OpenGL only handles
# power of two texture maps. This examples exercise's vtk's automatic
# power of two resampling).
#
# get the interactor ui
# create pipeline
#
# generate texture map (not power of two)
v16 = vtk.vtkVolume16Reader()
v16.SetDataDimensions(64,64)
v16.GetOutput().SetOrigin(0.0,0.0,0.0)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
v16.SetImageRange(1,93)
v16.SetDataSpacing(3.2,3.2,1.5)
extract = vtk.vtkExtractVOI()
extract.SetInputConnection(v16.GetOutputPort())
extract.SetVOI(32,32,0,63,0,92)
atext = vtk.vtkTexture()
atext.SetInputConnection(extract.GetOutputPort())
atext.InterpolateOn()
# gnerate plane to map texture on to
plane = vtk.vtkPlaneSource()
plane.SetXResolution(1)
plane.SetYResolution(1)
textureMapper = vtk.vtkPolyDataMapper()
textureMapper.SetInputConnection(plane.GetOutputPort())
textureActor = vtk.vtkActor()
textureActor.SetMapper(textureMapper)
textureActor.SetTexture(atext)
# Create the RenderWindow, Renderer
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(textureActor)
renWin.SetSize(250,250)
ren1.SetBackground(0.1,0.2,0.4)
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
