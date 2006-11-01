#!/usr/bin/env python

import vtk
from vtk.util.misc import vtkGetDataRoot

# create planes
# Create the RenderWindow, Renderer
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
pl3d = vtk.vtkPLOT3DReader()
pl3d.SetXYZFileName( vtkGetDataRoot() + '/Data/combxyz.bin' )
pl3d.SetQFileName( vtkGetDataRoot() + '/Data/combq.bin' )
pl3d.SetScalarFunctionNumber( 100 )
pl3d.SetVectorFunctionNumber( 202 )
pl3d.Update()

eg = vtk.vtkExtractGrid()
eg.SetInputConnection(pl3d.GetOutputPort())
eg.SetSampleRate(4,4,4)

gs = vtk.vtkGlyphSource2D()
gs.SetGlyphTypeToThickArrow()
gs.SetScale( 1 )
gs.FilledOff()
gs.CrossOff()

glyph = vtk.vtkGlyph3D()
glyph.SetInputConnection(eg.GetOutputPort())
glyph.SetSource(gs.GetOutput())
glyph.SetScaleFactor( 0.75 )

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(glyph.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

ren.AddActor(actor)

cam=ren.GetActiveCamera()
cam.SetClippingRange( 3.95297, 50 )
cam.SetFocalPoint( 8.88908, 0.595038, 29.3342 )
cam.SetPosition( -12.3332, 31.7479, 41.2387 )
cam.SetViewUp( 0.060772, -0.319905, 0.945498 )

renWin.Render()
