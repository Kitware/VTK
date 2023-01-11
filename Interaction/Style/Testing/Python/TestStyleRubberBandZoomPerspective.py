#!/usr/bin/env python
from __future__ import print_function

from vtkmodules.vtkFiltersCore import vtkIdFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkInteractionStyle import vtkInteractorStyleRubberBandZoom
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

class TestStyleRubberBandZoomPerspective(vtkmodules.test.Testing.vtkTest):

    def initPipeline(self):
        try:
            if self.pipelineInitialized:
                # default state
                self.style.LockAspectToViewportOff()
                self.style.CenterAtStartPositionOff()
                self.style.UseDollyForPerspectiveProjectionOn()

                # reset camera too
                self.renderer.ResetCamera()
                self.renderWindow.Render()
        except AttributeError:
            pass

        self.pipelineInitialized = True

        self.sphere = vtkSphereSource()
        self.idFilter = vtkIdFilter()
        self.mapper = vtkPolyDataMapper()
        self.actor = vtkActor()

        self.idFilter.PointIdsOff()
        self.idFilter.CellIdsOn()
        self.idFilter.SetInputConnection(self.sphere.GetOutputPort())

        self.mapper.SetInputConnection(self.idFilter.GetOutputPort())
        self.mapper.SetColorModeToMapScalars()
        self.mapper.SetScalarModeToUseCellFieldData()
        self.mapper.SelectColorArray("vtkIdFilter_Ids")
        self.mapper.UseLookupTableScalarRangeOff()
        self.mapper.SetScalarRange(0, 95)
        self.actor.SetMapper(self.mapper)

        self.renderer = vtkRenderer()
        self.renderer.AddActor(self.actor)

        self.renderWindow = vtkRenderWindow()
        self.renderWindow.AddRenderer(self.renderer)

        self.iren = vtkRenderWindowInteractor()
        self.iren.SetRenderWindow(self.renderWindow)

        self.style = vtkInteractorStyleRubberBandZoom()
        self.iren.SetInteractorStyle(self.style)

        self.renderer.GetActiveCamera().SetPosition(0, 0, -1)
        self.renderer.ResetCamera()
        self.renderWindow.Render()

    def interact(self):
        self.iren.SetEventInformationFlipY(150, 150, 0, 0, "0", 0, "0")
        self.iren.InvokeEvent("LeftButtonPressEvent")
        self.iren.SetEventInformationFlipY(192, 182, 0, 0, "0", 0, "0")
        self.iren.InvokeEvent("MouseMoveEvent")
        self.iren.InvokeEvent("LeftButtonReleaseEvent")

    def compare(self, suffix):
        img_file = "TestStyleRubberBandZoomPerspective-%s.png" % suffix
        vtkmodules.test.Testing.compareImage(self.renderWindow,
                vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

    def testDefault(self):
        print("testDefault")
        self.initPipeline()
        self.interact()
        self.compare("Default")

    def testLockAspect(self):
        print("testLockAspect")
        self.initPipeline()
        self.style.LockAspectToViewportOn()
        self.interact()
        self.compare("LockAspect")

    def testCenterAtStartPosition(self):
        print("testCenterAtStartPosition")
        self.initPipeline()
        self.style.CenterAtStartPositionOn()
        self.interact()
        self.compare("CenterAtStartPosition")

    def testCenterAtStartPositionAndLockAspect(self):
        print("testCenterAtStartPositionAndLockAspect")
        self.initPipeline()
        self.style.CenterAtStartPositionOn()
        self.style.LockAspectToViewportOn()
        self.interact()
        self.compare("CenterAtStartPositionAndLockAspect")

    def testParaViewWay(self):
        print("testParaViewWay")
        self.initPipeline()
        self.style.CenterAtStartPositionOn()
        self.style.LockAspectToViewportOn()
        self.style.UseDollyForPerspectiveProjectionOff()
        self.interact()
        self.compare("ParaViewWay")

if __name__ == "__main__":
    vtkmodules.test.Testing.main([(TestStyleRubberBandZoomPerspective, 'test')])
