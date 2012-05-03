#!/usr/bin/env python

# This example shows how to generate and manipulate texture coordinates.
# A random cloud of points is generated and then triangulated with
# vtkDelaunay3D. Since these points do not have texture coordinates,
# we generate them with vtkTextureMapToCylinder.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Begin by generating 25 random points in the unit sphere.
sphere = vtk.vtkPointSource()
sphere.SetNumberOfPoints(25)

# Triangulate the points with vtkDelaunay3D. This generates a convex hull
# of tetrahedron.
delny = vtk.vtkDelaunay3D()
delny.SetInputConnection(sphere.GetOutputPort())
delny.SetTolerance(0.01)

# The triangulation has texture coordinates generated so we can map
# a texture onto it.
tmapper = vtk.vtkTextureMapToCylinder()
tmapper.SetInputConnection(delny.GetOutputPort())
tmapper.PreventSeamOn()

# We scale the texture coordinate to get some repeat patterns.
xform = vtk.vtkTransformTextureCoords()
xform.SetInputConnection(tmapper.GetOutputPort())
xform.SetScale(4, 4, 1)

# vtkDataSetMapper internally uses a vtkGeometryFilter to extract the
# surface from the triangulation. The output (which is vtkPolyData) is
# then passed to an internal vtkPolyDataMapper which does the
# rendering.
mapper = vtk.vtkDataSetMapper()
mapper.SetInputConnection(xform.GetOutputPort())

# A texture is loaded using an image reader. Textures are simply images.
# The texture is eventually associated with an actor.
bmpReader = vtk.vtkBMPReader()
bmpReader.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")
atext = vtk.vtkTexture()
atext.SetInputConnection(bmpReader.GetOutputPort())
atext.InterpolateOn()
triangulation = vtk.vtkActor()
triangulation.SetMapper(mapper)
triangulation.SetTexture(atext)

# Create the standard rendering stuff.
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(triangulation)
ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)

iren.Initialize()
renWin.Render()
iren.Start()
