from vtkmodules.test import Testing as vtkTesting


class TestUpdateStateFromObject(vtkTesting.vtkTest):

    def test1(self):
        from vtkmodules.vtkSerializationManager import vtkObjectManager
        from vtkmodules.vtkRenderingCore import vtkCamera
        # from vtkmodules.vtkCommonCore import vtkLogger # uncomment for serialize prints
        import json

        manager = vtkObjectManager()
        manager.Initialize()

        # Scenario 1:
        # An object is registered and serialized. Later certain properties are modified on the object.
        # Call UpdateStateFromObject() with modified object. Verify the state's properties match
        # with the object's properties.

        camera = vtkCamera()

        # vtkLogger.SetStderrVerbosity(vtkLogger.VERBOSITY_TRACE) # uncomment for serialize prints
        manager.RegisterObject(camera)
        manager.UpdateStatesFromObjects()

        # Change object properties.
        camera.SetPosition(0, 0.2, 1.3)
        camera.SetViewAngle(60)

        # Sync up the state with object.
        manager.UpdateStateFromObject(manager.GetId(camera))

        camState = json.loads(manager.GetState(1))
        self.assertListEqual(camState["Position"], [0, 0.2, 1.3])
        self.assertEqual(camState["ViewAngle"], 60)


    def test2(self):
        from vtkmodules.vtkSerializationManager import vtkObjectManager
        from vtkmodules.vtkRenderingCore import vtkCamera
        import json

        manager = vtkObjectManager()
        manager.Initialize()

        # Scenario 1:
        # A state is registered and deserialized. Later certain properties are modified on the object.
        # Call UpdateStateFromObject() with modified object. Verify the state's properties match
        # with the object's properties.

        camState = {"Id": 1, "ClassName": "vtkCamera", "SuperClassNames": ["vtkObject"],
                    "vtk-object-manager-kept-alive": True, }
        manager.RegisterState(json.dumps(camState))
        manager.UpdateObjectsFromStates()

        # Change object properties.
        camera = manager.GetObjectAtId(1)
        camera.SetPosition(0, 0.2, 1.3)
        camera.SetViewAngle(60)

        # Sync up the state with object.
        manager.UpdateStateFromObject(manager.GetId(camera))

        camState = json.loads(manager.GetState(1))
        self.assertListEqual(camState["Position"], [0, 0.2, 1.3])
        self.assertEqual(camState["ViewAngle"], 60)


if __name__ == "__main__":
    vtkTesting.main([(TestUpdateStateFromObject, 'test')])
