#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkFiltersCore import vtkGlyph3D
from vtkmodules.vtkFiltersSources import (
    vtkConeSource,
    vtkSphereSource,
)
from vtkmodules.vtkRenderingCore import (
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingLOD import vtkLODActor
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class StyleBaseSpike(object):

    class Colors(object):
        '''
            Provides some wrappers for using color names.
        '''
        def __init__(self):
            '''
                Define a single instance of the NamedColors class here.
            '''
            self.namedColors = vtkNamedColors()

        def GetRGBColor(self, colorName):
            '''
                Return the red, green and blue components for a
                color as doubles.
            '''
            rgb = [0.0, 0.0, 0.0] # black
            self.namedColors.GetColorRGB(colorName, rgb)
            return rgb

        def GetRGBAColor(self, colorName):
            '''
                Return the red, green, blue and alpha
                components for a color as doubles.
            '''
            rgba = [0.0, 0.0, 0.0, 1.0] # black
            self.namedColors.GetColor(colorName, rgba)
            return rgba

    def __init__(self, ren, renWin, iren):

        self.ren = ren
        self.renWin = renWin
        self.iren = iren

        colors = self.Colors()

        self.renWin.AddRenderer(self.ren)

        self.iren.SetRenderWindow(self.renWin)
        self.iren.SetDesiredUpdateRate(.00001)

        # Create a sphere source and actor

        sphere = vtkSphereSource()

        sphereMapper = vtkPolyDataMapper()
        sphereMapper.SetInputConnection(sphere.GetOutputPort())

        sphereActor = vtkLODActor()
        sphereActor.SetMapper(sphereMapper)

        sphereActor.GetProperty().SetDiffuseColor(colors.GetRGBColor('banana'))
        sphereActor.GetProperty().SetSpecular(.4)
        sphereActor.GetProperty().SetSpecularPower(20)

        # Create the spikes using a cone source and the sphere source

        cone = vtkConeSource()
        cone.SetResolution(20)

        glyph = vtkGlyph3D()
        glyph.SetInputConnection(sphere.GetOutputPort())
        glyph.SetSourceConnection(cone.GetOutputPort())
        glyph.SetVectorModeToUseNormal()
        glyph.SetScaleModeToScaleByVector()
        glyph.SetScaleFactor(0.25)

        spikeMapper = vtkPolyDataMapper()
        spikeMapper.SetInputConnection(glyph.GetOutputPort())

        spikeActor = vtkLODActor()
        spikeActor.SetMapper(spikeMapper)

        spikeActor.GetProperty().SetDiffuseColor(colors.GetRGBColor('tomato'))
        spikeActor.GetProperty().SetSpecular(.4)
        spikeActor.GetProperty().SetSpecularPower(20)

        # Add the actors to the renderer, set the background and size

        self.ren.AddActor(sphereActor)
        self.ren.AddActor(spikeActor)
        self.ren.SetBackground(0.1, 0.2, 0.4)

        self.renWin.SetSize(300, 300)

        # Render the image

        self.ren.ResetCamera()
        cam1 = ren.GetActiveCamera()
        cam1.Zoom(1.4)
        cam1.Azimuth(30)
        cam1.Elevation(30)

        self.renWin.Render()

class TestStyleBaseSpike(vtkmodules.test.Testing.vtkTest):

    def testStyleBaseSpike(self):
        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        iRen = vtkRenderWindowInteractor()

        styleBaseSpike = StyleBaseSpike(ren, renWin, iRen)

        # render and interact with data
        renWin.Render()

        img_file = "TestStyleBaseSpike.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestStyleBaseSpike, 'test')])
