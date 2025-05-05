from vtkmodules.test import Testing as vtkTesting

class TestFilteredUpdateStatesFromObjects(vtkTesting.vtkTest):
    def test(self):
        from vtkmodules.vtkSerializationManager import vtkObjectManager
        from vtkmodules.vtkRenderingCore import vtkRenderWindow, vtkRenderer, vtkRenderWindowInteractor, vtkPolyDataMapper, vtkActor
        from vtkmodules.vtkFiltersSources import vtkSphereSource, vtkConeSource
        # from vtkmodules.vtkCommonCore import vtkLogger
        import json

        sphereActor = vtkActor(mapper = (vtkSphereSource() >> vtkPolyDataMapper()).last)
        coneActor = vtkActor(mapper = (vtkConeSource() >> vtkPolyDataMapper()).last)

        sphereRenderWindow = vtkRenderWindow()
        sphereRenderer = vtkRenderer()
        sphereRenderWindow.AddRenderer(sphereRenderer)
        sphereRenderer.AddActor(sphereActor)

        coneRenderWindow = vtkRenderWindow()
        coneRenderer = vtkRenderer()
        coneRenderWindow.AddRenderer(coneRenderer)
        coneRenderer.AddActor(coneActor)

        sphereInteractor = vtkRenderWindowInteractor(render_window = sphereRenderWindow)
        coneInteractor = vtkRenderWindowInteractor(render_window = coneRenderWindow)

        sphereInteractor.Render()
        coneInteractor.Render()

        manager = vtkObjectManager()
        manager.Initialize()

        # manager.SetObjectManagerLogVerbosity(vtkLogger.VERBOSITY_WARNING)
        # manager.serializer.SetSerializerLogVerbosity(vtkLogger.VERBOSITY_WARNING)

        sphereRenderWindowId = manager.RegisterObject(sphereRenderWindow)
        coneRenderWindowId = manager.RegisterObject(coneRenderWindow)

        manager.UpdateStatesFromObjects()

        # Keep track of the property IDs for verification later
        spherePropertyId = manager.GetId(sphereActor.property)
        conePropertyId = manager.GetId(coneActor.property)

        sphereActor.property.opacity = 0.5  # Change sphere opacity to see if it gets updated correctly
        manager.UpdateStatesFromObjects([sphereRenderWindowId])
        self.assertIn("vtk-object-manager-kept-alive", json.loads(manager.GetState(sphereRenderWindowId)), "vtk-object-manager-kept-alive key not found in sphere render window state")
        # verify that the sphere opacity change is reflected in the state
        self.assertEqual(json.loads(manager.GetState(spherePropertyId))['Opacity'], 0.5)
        sphereActor.property.opacity = 1.0  # Change sphere opacity back to original value

        coneActor.property.opacity = 0.3  # Change cone opacity to see if it gets updated correctly
        manager.UpdateStatesFromObjects([coneRenderWindowId])
        self.assertIn("vtk-object-manager-kept-alive", json.loads(manager.GetState(coneRenderWindowId)), "vtk-object-manager-kept-alive key not found in cone render window state")
        # verify that the cone opacity change is reflected in the state
        self.assertEqual(json.loads(manager.GetState(conePropertyId))['Opacity'], 0.3)

        # Since we only updated the cone render window, the sphere's opacity should not have changed
        self.assertEqual(json.loads(manager.GetState(spherePropertyId))['Opacity'], 0.5)

if __name__ == "__main__":
    vtkTesting.main([(TestFilteredUpdateStatesFromObjects, 'test')])
