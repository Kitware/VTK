#!/usr/bin/env python

# This example demonstrates the use of glyphing. We also use a mask filter
# to select a subset of points to glyph.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Read a data file. This originally was a Cyberware laser digitizer scan 
# of Fran J.'s face. Surface normals are generated based on local geometry
# (i.e., the polygon normals surrounding eash point are averaged). We flip
# the normals because we want them to point out from Fran's face.
fran = vtk.vtkPolyDataReader()
fran.SetFileName(VTK_DATA_ROOT + "/Data/fran_cut.vtk")
normals = vtk.vtkPolyDataNormals()
normals.SetInputConnection(fran.GetOutputPort())
normals.FlipNormalsOn()
franMapper = vtk.vtkPolyDataMapper()
franMapper.SetInputConnection(normals.GetOutputPort())
franActor = vtk.vtkActor()
franActor.SetMapper(franMapper)
franActor.GetProperty().SetColor(1.0, 0.49, 0.25)

# We subsample the dataset because we want to glyph just a subset of
# the points. Otherwise the display is cluttered and cannot be easily
# read. The RandonModeOn and SetOnRatio combine to random select one out
# of every 10 points in the dataset.
ptMask = vtk.vtkMaskPoints()
ptMask.SetInputConnection(normals.GetOutputPort())
ptMask.SetOnRatio(10)
ptMask.RandomModeOn()

# In this case we are using a cone as a glyph. We transform the cone so
# its base is at 0,0,0. This is the point where glyph rotation occurs.
cone = vtk.vtkConeSource()
cone.SetResolution(6)
transform = vtk.vtkTransform()
transform.Translate(0.5, 0.0, 0.0)
transformF = vtk.vtkTransformPolyDataFilter()
transformF.SetInputConnection(cone.GetOutputPort())
transformF.SetTransform(transform)

# vtkGlyph3D takes two inputs: the input point set (SetInput) which can be
# any vtkDataSet; and the glyph (SetSource) which must be a vtkPolyData.
# We are interested in orienting the glyphs by the surface normals that
# we previosuly generated.
glyph = vtk.vtkGlyph3D()
glyph.SetInputConnection(ptMask.GetOutputPort())
glyph.SetSource(transformF.GetOutput())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.004)
spikeMapper = vtk.vtkPolyDataMapper()
spikeMapper.SetInputConnection(glyph.GetOutputPort())
spikeActor = vtk.vtkActor()
spikeActor.SetMapper(spikeMapper)
spikeActor.GetProperty().SetColor(0.0, 0.79, 0.34)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(franActor)
ren.AddActor(spikeActor)

renWin.SetSize(500, 500)
ren.SetBackground(0.1, 0.2, 0.4)

# Set a nice camera position.
ren.ResetCamera()
cam1 = ren.GetActiveCamera()
cam1.Zoom(1.4)
cam1.Azimuth(110)

iren.Initialize()
renWin.Render()
iren.Start()
