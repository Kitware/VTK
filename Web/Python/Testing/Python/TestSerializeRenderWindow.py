import json
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderer,
)
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.web import render_window_serializer as rws
from vtkmodules.test import Testing

class TestSerializeRenderWindow(Testing.vtkTest):
    def testSerializeRenderWindow(self):
        cone = vtkConeSource()

        coneMapper = vtkPolyDataMapper()
        coneMapper.SetInputConnection(cone.GetOutputPort())

        coneActor = vtkActor()
        coneActor.SetMapper(coneMapper)

        ren = vtkRenderer()
        ren.AddActor(coneActor)
        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)

        ren.ResetCamera()
        renWin.Render()

        # Exercise some of the serialization functionality and make sure it
        # does not generate a stack trace
        context = rws.SynchronizationContext()
        rws.initializeSerializers()
        jsonData = rws.serializeInstance(None, renWin, rws.getReferenceId(renWin), context, 0)

        # jsonStr = json.dumps(jsonData)
        # print jsonStr
        # print len(jsonStr)

if __name__ == "__main__":
    Testing.main([(TestSerializeRenderWindow, 'test')])
