#!/usr/bin/env python
#
# All Plot3D scalar functions
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
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow( renWin )

scalarLabels = [ "Density", "Pressure", "Temperature", "Enthalpy",
                 "Internal_Energy", "Kinetic_Energy", "Velocity_Magnitude",
                 "Stagnation_Energy", "Entropy", "Swirl" ]
scalarFunctions = [ 100, 110, 120, 130, 140, 144, 153, 163, 170, 184 ]

camera = vtkCamera()
light  = vtkLight()
math   = vtkMath()

i = 0
pl3ds        = {}
planes       = {}
mappers      = {}
texts        = {}
text_mappers = {}
actors       = {}
ren          = {}

for scalarFunction in scalarFunctions:
  pl3ds[ scalarFunction ] = pl3d = vtkPLOT3DReader()

  pl3d.SetXYZFileName( VTK_DATA + "/bluntfinxyz.bin" )
  pl3d.SetQFileName  ( VTK_DATA + "/bluntfinq.bin"   )
  pl3d.SetScalarFunctionNumber( scalarFunction )
  pl3d.Update()
  
  planes[ scalarFunction ] = plane = vtkStructuredGridGeometryFilter()
  plane.SetInput( pl3d.GetOutput() )
  plane.SetExtent( 25, 25, 0, 100, 0, 100 )

  mappers[ scalarFunction ] = mapper = vtkPolyDataMapper()
  mapper.SetInput( plane.GetOutput() )
  mapper.SetScalarRange( 
    pl3d.GetOutput().GetPointData().GetScalars().GetRange() )

  actors[ scalarFunction ] = actor = vtkActor()
  actor.SetMapper( mapper )

  ren[ scalarFunction ] = rend = vtkRenderer()
  rend.SetActiveCamera( camera )
  rend.AddLight( light )
  renWin.AddRenderer( rend )
  rend.SetBackground( math.Random( 0.5, 1 ),
                      math.Random( 0.5, 1 ),
                      math.Random( 0.5, 1 ) )

  rend.AddActor( actor )

  text_mappers[ scalarFunction ] = text_map = vtkTextMapper()
  text_map.SetInput( scalarLabels[i] )
  text_map.SetFontSize( 10 )
  text_map.SetFontFamilyToArial()
  
  texts[ scalarFunction ] = text = vtkActor2D()
  text.SetMapper( text_map )
  text.SetPosition( 2, 3 )
  text.GetProperty().SetColor( 0, 0, 0 )

  rend.AddActor2D( text )

  i = i + 1

#
# now layout renderers
( column, row    ) = ( 1,1 )
( deltaX, deltaY ) = ( 1.0/5.0, 1.0/2.0 )

for scalarFunction in scalarFunctions:
  ren[ scalarFunction ].SetViewport(
    (column - 1) * deltaX, (row - 1) * deltaY,
    (column    ) * deltaX, (row    ) * deltaY )

  column = column + 1
  if column > 5:
    column = 1
    row = row + 1

camera.SetViewUp    ( 0, 1, 0 )
camera.SetFocalPoint( 0, 0, 0 )
camera.SetPosition  ( 1, 0, 0 )
ren[ 100 ].ResetCamera()
camera.Dolly( 1.25 )

for scalarFunction in scalarFunctions:
  ren[ scalarFunction ].ResetCameraClippingRange()

light.SetPosition  ( camera.GetPosition()   )
light.SetFocalPoint( camera.GetFocalPoint() )

renWin.SetSize( 600, 180 )
renWin.Render()
#renWin.SaveImageAsPPM()

# enable user interface interactor
iren.Initialize()

iren.Start()
