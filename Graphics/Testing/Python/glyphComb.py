#!/usr/bin/env python

import sys

for i in range(0, len(sys.argv)):
    if sys.argv[i] == '-A' and i < len(sys.argv)-1:
        sys.path = sys.path + [sys.argv[i+1]]

from vtkpython import *

dl = vtkDebugLeaks()
dl.PromptUserOff()

# create planes
# Create the RenderWindow, Renderer
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
pl3d = vtkPLOT3DReader()
pl3d.SetXYZFileName( vtkGetDataRoot() + '/Data/combxyz.bin' )
pl3d.SetQFileName( vtkGetDataRoot() + '/Data/combq.bin' )
pl3d.SetScalarFunctionNumber( 100 )
pl3d.SetVectorFunctionNumber( 202 )
pl3d.Update()

eg = vtkExtractGrid()
eg.SetInput(pl3d.GetOutput())
eg.SetSampleRate(4,4,4)

gs = vtkGlyphSource2D()
gs.SetGlyphTypeToThickArrow()
gs.SetScale( 1 )
gs.FilledOff()
gs.CrossOff()

glyph = vtkGlyph3D()
glyph.SetInput(eg.GetOutput())
glyph.SetSource(gs.GetOutput())
glyph.SetScaleFactor( 0.75 )

mapper = vtkPolyDataMapper()
mapper.SetInput(glyph.GetOutput())

actor = vtkActor()
actor.SetMapper(mapper)

ren.AddActor(actor)

cam=ren.GetActiveCamera()
cam.SetClippingRange( 3.95297, 50 )
cam.SetFocalPoint( 8.88908, 0.595038, 29.3342 )
cam.SetPosition( -12.3332, 31.7479, 41.2387 )
cam.SetViewUp( 0.060772, -0.319905, 0.945498 )

renWin.Render()
retVal = vtkRegressionTestImage(renWin)
sys.exit( not retVal )
