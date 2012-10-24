#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# load in the image
#
pnmReader = vtk.vtkTIFFReader()
pnmReader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/beach.tif")
# "beach.tif" image contains ORIENTATION tag which is 
# ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF 
# reader parses this tag and sets the internal TIFF image 
# orientation accordingly.  To overwrite this orientation with a vtk
# convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
# SetOrientationType method with parameter value of 4.
pnmReader.SetOrientationType(4)
lum = vtk.vtkImageLuminance()
lum.SetInputConnection(pnmReader.GetOutputPort())
ia = vtk.vtkImageActor()
ia.GetMapper().SetInputConnection(lum.GetOutputPort())
# Add the actors to the renderer, set the background and size
ren1.AddActor(ia)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(400,400)
# render the image
renWin.Render()
# switch from greyscale input to RGB to test against an old bug
ia.GetMapper().SetInputConnection(pnmReader.GetOutputPort())
cam1 = ren1.GetActiveCamera()
cam1.Elevation(-30)
cam1.Roll(-20)
ren1.ResetCameraClippingRange()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
