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

# Create polar axes
polaxes = vtkPolarAxesActor()
polaxes.SetBounds( cylinder.GetOutput().GetBounds() )
polaxes.SetPole( pole )
polaxes.SetAutoScaleRadius( 0 )
polaxes.SetMaximumRadius( 4.5 )
polaxes.SetMinimumAngle( -180. )
polaxes.SetMaximumAngle( 270. )
polaxes.SetNumberOfRadialAxes( 10 )
polaxes.SetNumberOfPolarAxisTicks( 9 )
polaxes.SetAutoSubdividePolarAxis( 0 )
polaxes.SetCamera( camera )
polaxes.SetPolarLabelFormat( "%6.1f" )
polaxes.GetRadialAxesProperty().SetColor( .0, .0, 1. )
polaxes.GetPolarArcsProperty().SetColor( 1., .0, 0. )
polaxes.GetPolarAxisProperty().SetColor( 0., 1., 0. )
polaxes.GetPolarAxisTitleTextProperty().SetColor( 0., 1., 0. )
polaxes.GetPolarAxisLabelTextProperty().SetColor( 0., 1., 0. )
polaxes.SetDistanceLODThreshold( .4 )
polaxes.SetViewAngleLODThreshold( .2 )
polaxes.SetScreenSize( 8. )

# Create renderer
renderer = vtkRenderer()
renderer.AddViewProp( cylactor )
renderer.AddViewProp( polaxes )
renderer.GradientBackgroundOn()
renderer.SetBackground( .8, .8 ,.8 )
renderer.SetBackground2( 1., 1. ,1. )
renderer.SetActiveCamera( camera )

# Create render window
window = vtkRenderWindow()
window.AddRenderer( renderer )
window.SetSize( 500, 500 )

# Create interactor
interactor = vtkRenderWindowInteractor()
interactor.SetRenderWindow( window )

# Start interaction
window.Render()
interactor.Start()
