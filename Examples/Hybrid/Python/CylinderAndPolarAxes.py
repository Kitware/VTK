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

# Create mappers
mapper = vtkPolyDataMapper()
mapper.SetInputConnection( cylinder.GetOutputPort() )

# Create cylinder actor
cylactor = vtkActor()
cylactor.SetMapper( mapper )
cylactor.GetProperty().SetColor( .5, .5, .5 )
cylactor.SetOrigin( pole )
cylactor.RotateX( 90. )

# Create renderer
renderer = vtkRenderer()
renderer.GradientBackgroundOn()
renderer.SetBackground( .8, .8 ,.8 )
renderer.SetBackground2( 1., 1. ,1. )
renderer.SetActiveCamera( camera )

# Create polar axes
polaxes = vtkPolarAxesActor()
polaxes.SetPole( pole )
polaxes.SetAutoScaleRadius( 0 )
polaxes.SetMaximumRadius( 4.5 )
polaxes.SetMinimumAngle( -60. )
polaxes.SetMaximumAngle( 210. )
polaxes.SetNumberOfRadialAxes( 10 )
polaxes.AutoSubdividePolarAxisOff()
polaxes.SetNumberOfPolarAxisTicks( 8 )
polaxes.SetCamera( renderer.GetActiveCamera() )
polaxes.SetPolarLabelFormat( "%6.1f" )
polaxes.GetRadialAxesProperty().SetColor( .0, .0, 1. )
polaxes.GetPolarArcsProperty().SetColor( 1., .0, 0. )
polaxes.GetPolarAxisProperty().SetColor( 0., 1., 0. )
polaxes.GetPolarAxisTitleTextProperty().SetColor( 0., 1., 0. )
polaxes.GetPolarAxisLabelTextProperty().SetColor( 0., 1., 0. )
polaxes.SetEnableDistanceLOD( 0 )
polaxes.SetDistanceLODThreshold( .4 )
polaxes.SetEnableViewAngleLOD( 0 )
polaxes.SetViewAngleLODThreshold( .2 )
polaxes.SetScreenSize( 8. )

# Create render window
window = vtkRenderWindow()
renderer.AddViewProp( cylactor )
renderer.AddViewProp( polaxes )
window.AddRenderer( renderer )
window.SetSize( 500, 500 )

# Create interactor
interactor = vtkRenderWindowInteractor()
interactor.SetRenderWindow( window )

# Start interaction
window.Render()
polaxes.SetMinimumAngle( 60. )
polaxes.SetNumberOfPolarAxisTicks( 12 )

interactor.Start()
