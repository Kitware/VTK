from vtkmodules.test import Testing as vtkTesting
from vtkmodules.vtkSerializationManager import vtkObjectManager
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkRenderingCore import vtkActor, vtkPolyDataMapper

import vtkmodules.vtkRenderingOpenGL2 # noqa

import json

class TestNull(vtkTesting.vtkTest):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.om = vtkObjectManager()
        self.om.Initialize()

    def test_null_in_state(self):
        mesh_id = self.om.RegisterObject(vtkPolyData())
        self.om.UpdateStatesFromObjects()
        state = json.loads(self.om.GetState(mesh_id))
        self.assertIsNone(state['Points'])

    def test_invoke_with_null_in_args(self):
        actor = vtkActor()
        mapper = vtkPolyDataMapper()
        actor.SetMapper(mapper)
        actor_id = self.om.RegisterObject(actor)
        self.om.UpdateStatesFromObjects()
        self.om.Invoke(actor_id, "SetMapper", "[null]")
        self.assertIsNone(actor.GetMapper())

    def test_invoke_with_null_in_return(self):
        actor = vtkActor()
        actor_id = self.om.RegisterObject(actor)
        self.om.UpdateStatesFromObjects()
        ret = self.om.Invoke(actor_id, "GetMapper", "[]")
        self.assertIsNone(json.loads(ret))

if __name__ == "__main__":
    vtkTesting.main([(TestNull, 'test')])
