#!/usr/bin/env python
#
# This example demonstrates how to use a programmable source and how to use
# the special vtkDataSetToDataSet::GetOutput() methods (i.e., see
# vtkWarpScalar)
#

import os, math
from libVTKCommonPython import *
from libVTKGraphicsPython import *
from colors import *

# create pipeline - use a programmable source to compute Bessel function and
# generate a plane of quadrilateral polygons,
#
besselSource = vtkProgrammableSource()
besselSource.SetExecuteMethod( lambda source=besselSource: bessel(source) )

# Generate plane with Bessel function scalar values.
# It's interesting to compare this with vtkPlaneSource.
def bessel( source ):

  ( XRes, XOrigin, XWidth ) = ( 25, -5.0, 10.0 )
  ( YRes, YOrigin, YWidth ) = ( 40, -5.0, 10.0 )

  newPts   = vtkPoints()
  newPolys = vtkCellArray()
  derivs   = vtkScalars()

  # Compute points and scalars
  id = 0
  for j in range( YRes+1 ):
    x1 = YOrigin + float(j) / YRes * YWidth
    for i in range( XRes+1 ):
      x0 = XOrigin + float(i) / XRes * XWidth

      r     = math.sqrt( x0*x0 + x1*x1 )
      x2    = math.exp( -r ) * math.cos( 10.0 * r )
      deriv = -math.exp( -r ) * ( math.cos( 10.0*r ) +
                                  10.0 * math.sin( 10.0*r ) )

      newPts.InsertPoint( id, x0, x1, x2 )
      derivs.InsertScalar( id, deriv )
      id = id + 1
  
  # Generate polygon connectivity
  for j in range( YRes ):
    for i in range( XRes ):
      newPolys.InsertNextCell( 4 )
      id = i + j * (XRes+1)
      newPolys.InsertCellPoint( id )
      newPolys.InsertCellPoint( id + 1 )
      newPolys.InsertCellPoint( id + XRes + 2 )
      newPolys.InsertCellPoint( id + XRes + 1 )

  source.GetPolyDataOutput().SetPoints( newPts   )
  source.GetPolyDataOutput().SetPolys ( newPolys )
  source.GetPolyDataOutput().GetPointData().SetScalars( derivs )
  
  #newPts.Delete()           #reference counting - it's ok
  #newPolys.Delete()
  #derivs.Delete()
  
# warp plane
warp = vtkWarpScalar()
warp.SetInput( besselSource.GetPolyDataOutput() )
warp.XYPlaneOn()
warp.SetScaleFactor( 0.5 )

# mapper and actor
mapper = vtkPolyDataMapper()
mapper.SetInput( warp.GetPolyDataOutput() )

# NOTE:  The queried scalar range will be (0,1) because we haven't updated the
#   source yet (i.e. besselSource.Update()).  The full range is actually
#   around (-7.1,4.0).  (0,1) produces more colorful results however.
mapper.SetScalarRange( besselSource.GetPolyDataOutput().GetScalarRange() )
carpet = vtkActor()
carpet.SetMapper( mapper )

# assign our actor to the renderer

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer( ren1 )
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow( renWin )

ren1.AddActor( carpet )
renWin.SetSize( 500, 500 )

# render the image
#
ren1.GetActiveCamera().Zoom( 1.5 )
renWin.Render()
#renWin.SetFileName( "expCos2.tcl.ppm" )
#renWin.SaveImageAsPPM()

# enable user interface interactor
iren.Initialize()

iren.Start()
