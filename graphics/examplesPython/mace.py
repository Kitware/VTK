from libVTKCommonPython import *
from libVTKGraphicsPython import *

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer.New()
renWin = vtkRenderWindow.New()
renWin.AddRenderer( ren1 )
iren = vtkRenderWindowInteractor.New()
iren.SetRenderWindow( renWin )

# create a sphere source and actor
#
sphere = vtkSphereSource.New()
sphereMapper = vtkPolyDataMapper.New()
sphereMapper.SetInput( sphere.GetOutput() )
sphereActor = vtkActor.New()
sphereActor.SetMapper( sphereMapper )

# create the spikes using a cone source and the sphere source
#
cone = vtkConeSource.New()

glyph = vtkGlyph3D.New()
glyph.SetInput( sphere.GetOutput() )
glyph.SetSource(cone.GetOutput())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor( 0.25 )
glyph.ReleaseDataFlagOn()

spikeMapper = vtkPolyDataMapper.New()
spikeMapper.SetInput( glyph.GetOutput() )
spikeActor = vtkActor.New()
spikeActor.SetMapper( spikeMapper )

# Add the actors to the renderer,.Set the background and size
#
ren1.AddActor( sphereActor )
ren1.AddActor( spikeActor )
ren1.SetBackground( 0.1, 0.2, 0.4 )
renWin.SetSize( 300, 300 )

def foo():
  print "In Callback"
# end of foo

def foo2():
  print "  Rendering"
# end of foo2

def foo3():
  print "  Filtering"
# end of foo3

iren.SetUserMethod(foo)
ren1.SetStartRenderMethod(foo2)
glyph.SetStartMethod(foo3)


# render the image
#
iren.Initialize()

iren.Start()

