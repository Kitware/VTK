#!/usr/bin/env python
############################################################
from vtk import *
############################################################

# Create pole for camera aim and polar axes pole
pole = [1., 12., 3.]

# Create camera
camera = vtkCamera()
camera.SetClippingRange( 1.0, 100.0 )
camera.SetFocalPoint( pole )
camera.SetPosition( 10., 10., 13. )

# Create cylinder
cylinder = vtkCylinderSource()
cylinder.SetRadius( 6. )
cylinder.SetCenter( 1., 2., 3. )
cylinder.SetHeight( 15 )
cylinder.SetResolution( 32 )
cylinder.Update()

# Create mappers for surface and wireframe representations
mapperS = vtkPolyDataMapper()
mapperS.SetInputConnection( cylinder.GetOutputPort() )
mapperS.SetResolveCoincidentTopologyPolygonOffsetParameters( 0, 1 )
mapperS.SetResolveCoincidentTopologyToPolygonOffset()
mapperW = vtkPolyDataMapper()
mapperW.SetInputConnection( cylinder.GetOutputPort() )
mapperW.SetResolveCoincidentTopologyPolygonOffsetParameters( 1, 1 )
mapperW.SetResolveCoincidentTopologyToPolygonOffset()

# Create actor for surface representation
surfactor = vtkActor()
surfactor.SetMapper( mapperS )
surfactor.GetProperty().SetColor( .89, .66, .41 )
surfactor.SetOrigin( pole )
surfactor.RotateX( 90. )

# Create actor for wireframe representation
wireactor = vtkActor()
wireactor.SetMapper( mapperW )
wireactor.GetProperty().SetColor( .1, .1 , .1 )
wireactor.GetProperty().SetRepresentationToWireframe()
wireactor.SetOrigin( pole )
wireactor.RotateX( 90. )

# Create renderer
renderer = vtkRenderer()
renderer.SetActiveCamera( camera )
renderer.GradientBackgroundOn()
renderer.SetBackground( .2, .2 ,.2 )
renderer.SetBackground2( .8, .8 ,.8 )

# Create polar axes
polaxes = vtkPolarAxesActor()
polaxes.SetPole( pole )
polaxes.SetAutoScaleRadius( 0 )
polaxes.SetMaximumRadius( 4.5 )
polaxes.SetMinimumAngle( -60. )
polaxes.SetMaximumAngle( 210. )
polaxes.SetNumberOfRadialAxes( 10 )
polaxes.AutoSubdividePolarAxisOff()
polaxes.SetNumberOfPolarAxisTicks( 5 )
polaxes.SetCamera( renderer.GetActiveCamera() )
polaxes.SetPolarLabelFormat( "%6.1f" )
polaxes.GetRadialAxesProperty().SetColor( .0, .0, 1. )
polaxes.GetPolarArcsProperty().SetColor( 1., .0, 0. )
polaxes.GetPolarAxisProperty().SetColor( 0., 0, 0. )
polaxes.GetPolarAxisTitleTextProperty().SetColor( 0., 0., 0. )
polaxes.GetPolarAxisLabelTextProperty().SetColor( 0., 0., 0. )
polaxes.SetEnableDistanceLOD( 1 )
polaxes.SetDistanceLODThreshold( .6 )
polaxes.SetEnableViewAngleLOD( 1 )
polaxes.SetViewAngleLODThreshold( .4 )
polaxes.SetScreenSize( 8. )

# Create render window
window = vtkRenderWindow()
renderer.AddViewProp( surfactor )
renderer.AddViewProp( wireactor )
renderer.AddViewProp( polaxes )
window.AddRenderer( renderer )
window.SetSize( 500, 500 )

# Create interactor
interactor = vtkRenderWindowInteractor()
interactor.SetRenderWindow( window )

# Start interaction
window.Render()
polaxes.SetMinimumAngle( 40. )
polaxes.SetMaximumAngle( 220. )
polaxes.SetNumberOfRadialAxes( 10 )
polaxes.SetNumberOfPolarAxisTicks( 10 )

interactor.Start()
