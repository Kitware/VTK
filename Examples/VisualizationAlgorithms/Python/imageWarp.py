#!/usr/bin/env python

# This example shows how to combine data from both the imaging and
# graphics pipelines. The vtkMergeFilter is used to merge the data
# from each together.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Read in an image and compute a luminance value. The image is
# extracted as a set of polygons (vtkImageDataGeometryFilter). We then
# will warp the plane using the scalar (luminance) values.
reader = vtk.vtkBMPReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")
luminance = vtk.vtkImageLuminance()
luminance.SetInput(reader.GetOutput())
geometry = vtk.vtkImageDataGeometryFilter()
geometry.SetInput(luminance.GetOutput())
warp = vtk.vtkWarpScalar()
warp.SetInput(geometry.GetOutput())
warp.SetScaleFactor(-0.1)

# Use vtkMergeFilter to combine the original image with the warped
# geometry.
merge = vtk.vtkMergeFilter()
merge.SetGeometry(warp.GetOutput())
merge.SetScalars(reader.GetOutput())
mapper = vtk.vtkDataSetMapper()
mapper.SetInput(merge.GetOutput())
mapper.SetScalarRange(0, 255)
mapper.ImmediateModeRenderingOff()
actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Create renderer stuff
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(actor)
ren.GetActiveCamera().Azimuth(20)
ren.GetActiveCamera().Elevation(30)
ren.SetBackground(0.1, 0.2, 0.4)
ren.ResetCameraClippingRange()

renWin.SetSize(250, 250)

cam1 = ren.GetActiveCamera()
cam1.Zoom(1.4)

iren.Initialize()
renWin.Render()
iren.Start()
