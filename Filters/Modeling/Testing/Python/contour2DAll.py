#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkIOImage import vtkSLCReader
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

class contour2DAll(vtkmodules.test.Testing.vtkTest):

    def testContour2DAll(self):

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.SetMultiSamples(0)
        renWin.AddRenderer(ren)

        # create pipeline
        #
        slc = vtkSLCReader()
        slc.SetFileName(VTK_DATA_ROOT + "/Data/nut.slc")
        slc.Update()

        types = ["Char", "UnsignedChar", "Short", "UnsignedShort", "Int", "UnsignedInt", "Long", "UnsignedLong", "Float", "Double"]

        i = 3

        clip = list()
        cast = list()
        iso = list()
        mapper = list()
        actor = list()

        for idx, vtkType in enumerate(types):

            clip.append(vtkImageClip())
            clip[idx].SetInputConnection(slc.GetOutputPort())
            clip[idx].SetOutputWholeExtent(-1000, 1000, -1000, 1000, i, i)

            i += 2

            cast.append(vtkImageCast())
            eval("cast[idx].SetOutputScalarTypeTo" + vtkType)
            cast[idx].SetInputConnection(clip[idx].GetOutputPort())
            cast[idx].ClampOverflowOn()

            iso.append(vtkContourFilter())
            iso[idx] = vtkContourFilter()
            iso[idx].SetInputConnection(cast[idx].GetOutputPort())
            iso[idx].GenerateValues(1, 30, 30)

            mapper.append(vtkPolyDataMapper())
            mapper[idx].SetInputConnection(iso[idx].GetOutputPort())
            mapper[idx].SetColorModeToMapScalars()

            actor.append(vtkActor())
            actor[idx].SetMapper(mapper[idx])
            ren.AddActor(actor[idx])

        outline = vtkOutlineFilter()
        outline.SetInputConnection(slc.GetOutputPort())
        outlineMapper = vtkPolyDataMapper()
        outlineMapper.SetInputConnection(outline.GetOutputPort())
        outlineActor = vtkActor()
        outlineActor.SetMapper(outlineMapper)
        outlineActor.VisibilityOff()
        #
        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(outlineActor)
        ren.ResetCamera()
        ren.GetActiveCamera().SetViewAngle(30)
        ren.GetActiveCamera().Elevation(20)
        ren.GetActiveCamera().Azimuth(20)
        ren.GetActiveCamera().Zoom(1.5)
        ren.ResetCameraClippingRange()

        ren.SetBackground(0.9, .9, .9)
        renWin.SetSize(200, 200)

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "contour2DAll.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(contour2DAll, 'test')])
