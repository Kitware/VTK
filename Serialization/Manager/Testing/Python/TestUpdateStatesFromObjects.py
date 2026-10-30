from vtkmodules.test import Testing as vtkTesting
from vtkmodules.vtkCommonCore import vtkLogger
from pathlib import Path


class TestUpdateStatesFromObjects(vtkTesting.vtkTest):
    def setUp(self):
        import vtkmodules.vtkRenderingOpenGL2
        from vtkmodules.vtkSerializationManager import vtkObjectManager
        from vtkmodules.vtkFiltersSources import vtkSphereSource
        from vtkmodules.vtkRenderingCore import vtkActor, vtkPolyDataMapper, vtkRenderer, vtkRenderWindow

        self.manager = vtkObjectManager()
        self.manager.Initialize()

        self.sphere = vtkSphereSource()
        self.mapper = vtkPolyDataMapper()
        self.actor = vtkActor(mapper=self.mapper)
        self.sphere >> self.mapper
        self.renderer = vtkRenderer()
        self.window = vtkRenderWindow()
        self.renderer.AddActor(self.actor)
        self.window.AddRenderer(self.renderer)

        self.manager.serializer.SetSerializerLogVerbosity(vtkLogger.VERBOSITY_INFO)

        self.id_window = self.manager.RegisterObject(self.window)
        self.id_actor = self.manager.RegisterObject(self.actor)

    def test1(self):
        log = Path(vtkTesting.VTK_TEMP_DIR) / "TestUpdateStatesFromObjects.log.test1"
        vtkLogger.LogToFile(str(log), vtkLogger.TRUNCATE, vtkLogger.VERBOSITY_INFO)

        self.manager.UpdateStatesFromObjects([self.id_window, self.id_actor])
        self.assertIn(self.manager.GetId(self.mapper), self.manager.GetAllDependencies(self.id_window))

        vtkLogger.EndLogToFile(str(log))
        self.addCleanup(log.unlink)
        n = sum("{ Serialize objectBase=vtkOpenGLActor" in line for line in log.open())
        self.assertEqual(n, 1)

    def test2(self):
        log = Path(vtkTesting.VTK_TEMP_DIR) / "TestUpdateStatesFromObjects.log.test2"
        vtkLogger.LogToFile(str(log), vtkLogger.TRUNCATE, vtkLogger.VERBOSITY_INFO)

        self.manager.UpdateStatesFromObjects([self.id_actor, self.id_window])
        self.assertIn(self.manager.GetId(self.mapper), self.manager.GetAllDependencies(self.id_window))

        vtkLogger.EndLogToFile(str(log))
        self.addCleanup(log.unlink)
        n = sum("{ Serialize objectBase=vtkOpenGLActor" in line for line in log.open())
        self.assertEqual(n, 1)


if __name__ == "__main__":
    vtkTesting.main([(TestUpdateStatesFromObjects, 'test')])
