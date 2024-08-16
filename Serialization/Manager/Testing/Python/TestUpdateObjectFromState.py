from vtkmodules.test import Testing as vtkTesting


class TestUpdateObjectFromState(vtkTesting.vtkTest):

    def test1(self):
        from vtkmodules.vtkSerializationManager import vtkObjectManager
        import json

        # Scenario 1:
        # A state is registered and deserialized. Later certain properties are modified on the state.
        # Call UpdateObjectFromState() with modified state. Verify the real object's properties match
        # with the state's properties.

        manager = vtkObjectManager()
        manager.Initialize()

        camState = {"Id": 1, "ClassName": "vtkCamera", "SuperClassNames": ["vtkObject"],
                    "vtk-object-manager-kept-alive": True, }
        manager.RegisterState(json.dumps(camState))
        manager.UpdateObjectsFromStates()

        camState["Position"] = (0, 0.2, 1.3)
        camState["ViewAngle"] = 60

        manager.UpdateObjectFromState(json.dumps(camState))

        camera = manager.GetObjectAtId(1)
        self.assertTupleEqual(camera.GetPosition(), (0, 0.2, 1.3))
        self.assertEqual(camera.GetViewAngle(), 60)


    def test2(self):
        from vtkmodules.vtkSerializationManager import vtkObjectManager
        from vtkmodules.vtkRenderingCore import vtkCamera
        import json

        # Scenario 2:
        # An object is registered and serialized. Later certain properties are modified on the state.
        # Call UpdateObjectFromState() with modified state. Verify the real object's properties match
        # with the state's properties.

        manager = vtkObjectManager()
        manager.Initialize()

        camera = vtkCamera()
        manager.RegisterObject(camera)
        manager.UpdateStatesFromObjects()

        camState = json.loads(manager.GetState(1))
        camState["Position"] = (0, 0.2, 1.3)
        camState["ViewAngle"] = 60

        manager.UpdateObjectFromState(json.dumps(camState))

        self.assertTupleEqual(camera.GetPosition(), (0, 0.2, 1.3))
        self.assertEqual(camera.GetViewAngle(), 60)

if __name__ == "__main__":
    vtkTesting.main([(TestUpdateObjectFromState, 'test')])
