#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkCommonDataModel import (
    vtkCellLocator,
    vtkPointLocator,
)
from vtkmodules.vtkFiltersGeneral import (
    vtkOBBTree,
    vtkSpatialRepresentationFilter,
)
from vtkmodules.vtkIOGeometry import vtkSTLReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
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

class spatialRepAll(vtkmodules.test.Testing.vtkTest):

    def testspatialRepAll(self):

        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.SetMultiSamples(0)
        renWin.AddRenderer(ren)

        asource = vtkSTLReader()
        asource.SetFileName(VTK_DATA_ROOT + "/Data/42400-IDGH.stl")
        dataMapper = vtkPolyDataMapper()
        dataMapper.SetInputConnection(asource.GetOutputPort())
        model = vtkActor()
        model.SetMapper(dataMapper)
        model.GetProperty().SetColor(1, 0, 0)
        model.VisibilityOn()

        locators = ["vtkPointLocator", "vtkCellLocator", "vtkOBBTree"]

        locator = list()
        boxes = list()
        boxMapper = list()
        boxActor = list()

        for idx, vtkLocatorType in enumerate(locators):
            eval('locator.append(' + vtkLocatorType + '())')
            locator[idx].AutomaticOff()
            locator[idx].SetMaxLevel(3)

            boxes.append(vtkSpatialRepresentationFilter())
            boxes[idx].SetInputConnection(asource.GetOutputPort())
            boxes[idx].SetSpatialRepresentation(locator[idx])
            boxes[idx].SetGenerateLeaves(1)
            boxes[idx].Update()

            output = boxes[idx].GetOutput().GetBlock(boxes[idx].GetMaximumLevel() + 1)

            boxMapper.append(vtkPolyDataMapper())
            boxMapper[idx].SetInputData(output)

            boxActor.append(vtkActor())
            boxActor[idx].SetMapper(boxMapper[idx])
            boxActor[idx].AddPosition((idx + 1) * 15, 0, 0)

            ren.AddActor(boxActor[idx])


        ren.AddActor(model)
        ren.SetBackground(0.1, 0.2, 0.4)
        renWin.SetSize(400, 160)

        # render the image
        camera = vtkCamera()
        camera.SetPosition(148.579, 136.352, 214.961)
        camera.SetFocalPoint(151.889, 86.3178, 223.333)
        camera.SetViewAngle(30)
        camera.SetViewUp(0, 0, -1)
        camera.SetClippingRange(1, 100)
        ren.SetActiveCamera(camera)

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "spatialRepAll.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(spatialRepAll, 'test')])
