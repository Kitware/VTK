#!/usr/bin/env python
#
# All Plot3D vector functions
#

import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *
from colors import *

# Create the RenderWindow, Renderer and both Actors
#
renWin = vtkRenderWindow()
ren1 = vtkRenderer()
ren1.SetBackground( .8, .8, 2 )
renWin.AddRenderer( ren1 )
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow( renWin )

vectorLabels = [ "Velocity", "Vorticity", "Momentum", "Pressure_Gradient" ]
vectorFunctions = [ 200, 201, 202, 210 ]

camera = vtkCamera()
light  = vtkLight()

i = 0
pl3ds        = {}
planes       = {}
hogs         = {}
mappers      = {}
texts        = {}
text_mappers = {}
actors       = {}
ren          = {}

for vectorFunction in vectorFunctions:
  pl3ds[ vectorFunction ] = pl3d = vtkPLOT3DReader()

  pl3d.SetXYZFileName( VTK_DATA + "/bluntfinxyz.bin" )
  pl3d.SetQFileName  ( VTK_DATA + "/bluntfinq.bin"   )
  pl3d.SetVectorFunctionNumber( vectorFunction )
  pl3d.Update()
  
  planes[ vectorFunction ] = plane = vtkStructuredGridGeometryFilter()
  plane.SetInput( pl3d.GetOutput() )
  plane.SetExtent( 25, 25, 0, 100, 0, 100 )

  hogs[ vectorFunction ] = hog = vtkHedgeHog()
  hog.SetInput( plane.GetOutput() )
  hog.SetScaleFactor(
    1.0 / pl3d.GetOutput().GetPointData().GetVectors().GetMaxNorm() )

  mappers[ vectorFunction ] = mapper = vtkPolyDataMapper()
  mapper.SetInput( hog.GetOutput() )

  actors[ vectorFunction ] = actor = vtkActor()
  actor.SetMapper( mapper )

  ren[ vectorFunction ] = rend = vtkRenderer()
  rend.SetBackground( 0.5, 0.5, 0.5 )
  rend.SetActiveCamera( camera )
  rend.AddLight( light )
  renWin.AddRenderer( rend )

  rend.AddActor( actor )

  text_mappers[ vectorFunction ] = text_map = vtkTextMapper()
  text_map.SetInput( vectorLabels[i] )
  text_map.SetFontSize( 10 )
  text_map.SetFontFamilyToArial()
  
  texts[ vectorFunction ] = text = vtkActor2D()
  text.SetMapper( text_map )
  text.SetPosition( 2, 5 )
  text.GetProperty().SetColor( .3, 1, 1 )

  rend.AddActor2D( text )

  i = i + 1

#
# now layout renderers
( column, row    ) = ( 1,1 )
( deltaX, deltaY ) = ( 1.0/2.0, 1.0/2.0 )

for vectorFunction in vectorFunctions:
  ren[ vectorFunction ].SetViewport(
    (column - 1) * deltaX + deltaX * 0.05, (row - 1) * deltaY + deltaY * 0.05,
    (column    ) * deltaX - deltaX * 0.05, (row    ) * deltaY - deltaY * 0.05 )

  column = column + 1
  if column > 2:
    column = 1
    row = row + 1

camera.SetViewUp    ( 1, 0, 0 )
camera.SetFocalPoint( 0, 0, 0 )
camera.SetPosition  ( .4, -.5, -.75 )
ren[ 200 ].ResetCamera()
camera.Dolly( 1.25 )

for vectorFunction in vectorFunctions:
  ren[ vectorFunction ].ResetCameraClippingRange()

light.SetPosition  ( camera.GetPosition()   )
light.SetFocalPoint( camera.GetFocalPoint() )

renWin.SetSize( 350, 350 )
renWin.Render()
#renWin.SaveImageAsPPM()

# enable user interface interactor
iren.Initialize()

iren.Start()
