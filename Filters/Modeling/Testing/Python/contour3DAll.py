#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkIOLegacy import vtkStructuredPointsReader
from vtkmodules.vtkImagingCore import (
    vtkImageCast,
    vtkImageClip,
)
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
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()



class contour3DAll(vtkmodules.test.Testing.vtkTest):

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


    def testContour3DAll(self):

        # On older Macs, 10 is too low. Due to what looks like a driver bug
        # spectral lighting behaves sort of weird and produces small differences
        threshold = 30

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)

        # create pipeline
        #
        slc = vtkStructuredPointsReader()
        slc.SetFileName(VTK_DATA_ROOT + "/Data/ironProt.vtk")

        actorColors = ["flesh", "banana", "grey", "pink", "carrot", "gainsboro", "tomato", "gold", "thistle", "chocolate"]

        types = ["UnsignedChar", "Char", "Short", "UnsignedShort", "Int", "UnsignedInt", "Long", "UnsignedLong", "Float", "Double"]

        i = 1
        c = 0

        clip = list()
        cast = list()
        iso = list()
        mapper = list()
        actor = list()

        colors = self.Colors()

        for idx, vtkType in enumerate(types):
            clip.append(vtkImageClip())
            clip[idx].SetInputConnection(slc.GetOutputPort())
            clip[idx].SetOutputWholeExtent(-1000, 1000, -1000, 1000, i, i + 5)

            i += 5

            cast.append(vtkImageCast())
            eval("cast[idx].SetOutputScalarTypeTo" + vtkType)
            cast[idx].SetInputConnection(clip[idx].GetOutputPort())
            cast[idx].ClampOverflowOn()

            iso.append(vtkContourFilter())
            iso[idx].SetInputConnection(cast[idx].GetOutputPort())
            iso[idx].GenerateValues(1, 30, 30)
            iso[idx].ComputeScalarsOff()
            iso[idx].ComputeGradientsOff()

            mapper.append(vtkPolyDataMapper())
            mapper[idx].SetInputConnection(iso[idx].GetOutputPort())

            actor.append(vtkActor())
            actor[idx].SetMapper(mapper[idx])
            eval('actor[idx].GetProperty().SetDiffuseColor(colors.GetRGBColor("' + actorColors[idx] + '"))')
            actor[idx].GetProperty().SetSpecularPower(30)
            actor[idx].GetProperty().SetDiffuse(.7)
            actor[idx].GetProperty().SetSpecular(.5)

            c += 3

            ren.AddActor(actor[idx])


        outline = vtkOutlineFilter()
        outline.SetInputConnection(slc.GetOutputPort())
        outlineMapper = vtkPolyDataMapper()
        outlineMapper.SetInputConnection(outline.GetOutputPort())
        outlineActor = vtkActor()
        outlineActor.SetMapper(outlineMapper)
        outlineActor.VisibilityOff()

        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(outlineActor)
        ren.SetBackground(0.9, .9, .9)
        ren.ResetCamera()
        ren.GetActiveCamera().SetViewAngle(30)
        ren.GetActiveCamera().Elevation(20)
        ren.GetActiveCamera().Azimuth(20)
        ren.GetActiveCamera().Zoom(1.5)
        ren.ResetCameraClippingRange()

        renWin.SetSize(400, 400)

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()


        img_file = "contour3DAll.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(contour3DAll, 'test')])
