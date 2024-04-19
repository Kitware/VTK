#!/usr/bin/env python
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersCore import (
    vtkCleanPolyData,
    vtkHedgeHog,
)
from vtkmodules.vtkFiltersGeneral import vtkTransformPolyDataFilter
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkIOGeometry import vtkOBJReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# This differs from cowHair because it checks the "MergingOff" feature
# of vtkCleanPolyData....it should give the same result.
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read data
#
wavefront = vtkOBJReader()
wavefront.SetFileName(VTK_DATA_ROOT + "/Data/Viewpoint/cow.obj")
wavefront.Update()

cone = vtkConeSource()
cone.SetResolution(6)
cone.SetRadius(.1)

transform = vtkTransform()
transform.Translate(0.5, 0.0, 0.0)
transformF = vtkTransformPolyDataFilter()
transformF.SetInputConnection(cone.GetOutputPort())
transformF.SetTransform(transform)

# we just clean the normals for efficiency (keep down number of cones)
clean = vtkCleanPolyData()
clean.SetInputConnection(wavefront.GetOutputPort())
clean.PointMergingOff()

glyph = vtkHedgeHog()
glyph.SetInputConnection(clean.GetOutputPort())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleFactor(0.4)

hairMapper = vtkPolyDataMapper()
hairMapper.SetInputConnection(glyph.GetOutputPort())

hair = vtkActor()
hair.SetMapper(hairMapper)

cowMapper = vtkPolyDataMapper()
cowMapper.SetInputConnection(wavefront.GetOutputPort())
cow = vtkActor()
cow.SetMapper(cowMapper)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(cow)
ren1.AddActor(hair)
ren1.ResetCamera()
ren1.GetActiveCamera().Dolly(2)
ren1.GetActiveCamera().Azimuth(30)
ren1.GetActiveCamera().Elevation(30)
ren1.ResetCameraClippingRange()

# hair.GetProperty().SetDiffuseColor(saddle_brown)
# hair.GetProperty().SetAmbientColor(thistle)
# hair.GetProperty().SetAmbient(.3)
#
# cow.GetProperty().SetDiffuseColor(beige)
hair.GetProperty().SetDiffuseColor(GetRGBColor('saddle_brown'))
hair.GetProperty().SetAmbientColor(GetRGBColor('thistle'))
hair.GetProperty().SetAmbient(.3)

# Beige in vtkNamedColors() is compliant with
# http://en.wikipedia.org/wiki/Web_colors and is a different color.
# so we revert to the one used in colors.py.
# cow.GetProperty().SetDiffuseColor(GetRGBColor('beige'))
cow.GetProperty().SetDiffuseColor(163 / 255.0, 148 / 255.0, 128 / 255.0)

renWin.SetSize(320, 240)

ren1.SetBackground(.1, .2, .4)
iren.Initialize()
renWin.Render()

# render the image
#
#iren.Start()
