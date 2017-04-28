import json
import vtk
from vtk.web import render_window_serializer as rws
from vtk.test import Testing

class TestSerializeRenderWindow(Testing.vtkTest):
    def testSerializeRenderWindow(self):
        cone = vtk.vtkConeSource()

        coneMapper = vtk.vtkPolyDataMapper()
        coneMapper.SetInputConnection(cone.GetOutputPort())

        coneActor = vtk.vtkActor()
        coneActor.SetMapper(coneMapper)

        ren = vtk.vtkRenderer()
        ren.AddActor(coneActor)
        renWin = vtk.vtkRenderWindow()
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
