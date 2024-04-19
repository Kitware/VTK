#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkDelaunay3D
from vtkmodules.vtkFiltersSources import vtkPointSource
from vtkmodules.vtkFiltersTexture import (
    vtkTextureMapToCylinder,
    vtkTransformTextureCoords,
)
from vtkmodules.vtkIOImage import vtkBMPReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTexture,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Generate texture coordinates on a "random" sphere.
# create some random points in a sphere
#
sphere = vtkPointSource()
sphere.SetNumberOfPoints(25)
# triangulate the points
#
del1 = vtkDelaunay3D()
del1.SetInputConnection(sphere.GetOutputPort())
del1.SetTolerance(0.01)
# texture map the sphere (using cylindrical coordinate system)
#
tmapper = vtkTextureMapToCylinder()
tmapper.SetInputConnection(del1.GetOutputPort())
tmapper.PreventSeamOn()
xform = vtkTransformTextureCoords()
xform.SetInputConnection(tmapper.GetOutputPort())
xform.SetScale(4,4,1)
mapper = vtkDataSetMapper()
mapper.SetInputConnection(xform.GetOutputPort())
# load in the texture map and assign to actor
#
bmpReader = vtkBMPReader()
bmpReader.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")
atext = vtkTexture()
atext.SetInputConnection(bmpReader.GetOutputPort())
atext.InterpolateOn()
triangulation = vtkActor()
triangulation.SetMapper(mapper)
triangulation.SetTexture(atext)
# Create rendering stuff
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(triangulation)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
