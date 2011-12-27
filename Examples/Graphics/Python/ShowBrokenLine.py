############################################################
from vtk import *
############################################################

# Create sources
arc = vtkArcSource()
arc.SetCenter( 0, 0, 0 )
arc.SetPoint1( 1, 0, 0 )
arc.SetPoint2( -1, 0, 0 )
arc.SetResolution( 32 )
sphere = vtkSphereSource()
sphere.SetRadius( 0.05 )
sphere.SetCenter( 0, 0, 0 )
sphere.SetPhiResolution( 16 )
sphere.SetThetaResolution( 16 )

# Create mappers
mapper1 = vtkPolyDataMapper()
mapper1.SetInput( arc.GetOutput() )

mapper2 = vtkPolyDataMapper()
mapper2.SetInputConnection( sphere.GetOutputPort() )

# Create actors
actor1 = vtkActor()
actor1.SetMapper( mapper1 )
actor1.GetProperty().SetColor( 1., 0., 0. )
actor2 = vtkActor()
actor2.SetMapper( mapper2 )

# Create renderer
renderer = vtkRenderer()
renderer.AddViewProp( actor1 )
renderer.AddViewProp( actor2 )
renderer.SetBackground( .3, .4 ,.5 )

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
