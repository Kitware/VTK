from vtkmodules.test import Testing as vtkTesting
from vtkmodules.vtkCommonCore import vtkLogger
from vtkmodules.vtkSerializationManager import vtkObjectManager
from vtkmodules.vtkRenderingCore import vtkActor, vtkRenderer

# Ensure the OpenGL2 rendering backend is loaded.
import vtkmodules.vtkRenderingOpenGL2 # noqa

class TestRendererActorCollection(vtkTesting.vtkTest):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._renderer = vtkRenderer()
        self._prop1 = vtkActor(origin=(1,2,3))
        self._prop2 = vtkActor(origin=(4,5,6))
        self.renderer_id = -1
        self.mock_server_object_manager = vtkObjectManager()
        self.mock_server_object_manager.Initialize()
        self.mock_client_object_manager = vtkObjectManager()
        self.mock_client_object_manager.Initialize()

    def setUp(self):
        self.renderer_id = self.mock_server_object_manager.RegisterObject(self._renderer)

    def serialize(self):

        self.mock_server_object_manager.UpdateStatesFromObjects()
        object_ids = self.mock_server_object_manager.GetAllDependencies(0)

        return map(self.mock_server_object_manager.GetState, object_ids)

    def deserialize(self, states):

        for state in states:
            self.mock_client_object_manager.RegisterState(state)

        self.mock_client_object_manager.UpdateObjectsFromStates()
        self.client_side_renderer = self.mock_client_object_manager.GetObjectAtId(self.renderer_id)

    def test(self):
        # update state with empty scene
        self.deserialize(self.serialize())
        self.assertEqual(self.client_side_renderer.GetViewProps().GetNumberOfItems(), 0)
        # update state of renderer
        self.mock_client_object_manager.UpdateStateFromObject(self.renderer_id)

        for i_pass in range(2):
            # add props to scene
            self._renderer.AddActor(self._prop1)
            self._renderer.AddActor(self._prop2)
            self.deserialize(self.serialize())
            self.assertEqual(self.client_side_renderer.GetViewProps().GetNumberOfItems(), 2)
            for prop in self.client_side_renderer.GetViewProps():
                self.assertIn(prop.GetOrigin(), [(1,2,3), (4,5,6)])
                self.assertIsInstance(prop, vtkActor)
            print(f"{i_pass=} Adding actor passed.")
            # remove prop from scene
            self._renderer.RemoveActor(self._prop1)
            self.deserialize(self.serialize())
            self.assertEqual(self.client_side_renderer.GetViewProps().GetNumberOfItems(), 1)
            for prop in self.client_side_renderer.GetViewProps():
                self.assertEqual(prop.GetOrigin(), (4,5,6))
                self.assertIsInstance(prop, vtkActor)
            print(f"{i_pass=} Removing actor passed.")
            self._renderer.RemoveActor(self._prop2)
            self.deserialize(self.serialize())
            self.assertEqual(self.client_side_renderer.GetViewProps().GetNumberOfItems(), 0)
            print(f"{i_pass=} Removing last actor passed.")

    def tearDown(self):
        self.mock_server_object_manager.Clear()
        self.mock_client_object_manager.Clear()

if __name__ == "__main__":
    vtkLogger.Init()
    vtkTesting.main([(TestRendererActorCollection, 'test')])
