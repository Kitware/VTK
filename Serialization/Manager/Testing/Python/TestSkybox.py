from vtkmodules.test import Testing as vtkPyTesting
from vtkmodules.vtkTestingRendering import vtkTesting as vtkCppTesting
from vtkmodules.vtkSerializationManager import vtkObjectManager
from vtkmodules.vtkCommonCore import vtkFloatArray, vtkLogger
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkCommonDataModel import vtkImageData
from vtkmodules.vtkRenderingCore import vtkTexture, vtkRenderWindow, vtkSkybox, vtkRenderer, vtkActor, vtkPolyDataMapper, vtkRenderWindowInteractor
from vtkmodules.vtkRenderingOpenGL2 import vtkOpenGLSkybox
from vtkmodules.vtkIOImage import vtkHDRReader


class TestSkybox(vtkPyTesting.vtkTest):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        pass

    def setUp(self):
        self.id_rwi = 0

        self.render_window = vtkRenderWindow()
        renderer = vtkRenderer()
        self.render_window.AddRenderer(renderer)
        interactor = vtkRenderWindowInteractor()
        self.render_window.SetInteractor(interactor)

        skybox = vtkOpenGLSkybox()
        hdr_reader = vtkHDRReader()
        hdr_reader.SetFileName(
            vtkPyTesting.VTK_DATA_ROOT + "/Data/spiaggia_di_mondello_1k.hdr")
        texture = vtkTexture()
        texture.SetColorModeToDirectScalars()
        texture.MipmapOn()
        texture.InterpolateOn()
        texture.SetInputConnection(hdr_reader.GetOutputPort())

        renderer.UseImageBasedLightingOn()
        renderer.SetEnvironmentTexture(texture)

        skybox.SetFloorRight(0.0, 0.0, 1.0)
        skybox.SetProjection(vtkSkybox.Sphere)
        skybox.SetTexture(texture)

        renderer.AddActor(skybox)

        sphere = vtkSphereSource()
        sphere.SetThetaResolution(75)
        sphere.SetPhiResolution(75)

        pdSphere = vtkPolyDataMapper()
        pdSphere.SetInputConnection(sphere.GetOutputPort())

        for i in range(6):
            actorSphere = vtkActor()
            actorSphere.SetPosition(i, 0.0, 0.0)
            actorSphere.SetMapper(pdSphere)
            actorSphere.GetProperty().SetInterpolationToPBR()
            actorSphere.GetProperty().SetMetallic(1.0)
            actorSphere.GetProperty().SetRoughness(i / 5.0)
            renderer.AddActor(actorSphere)

    def serialize(self):

        manager = vtkObjectManager()
        manager.Initialize()
        self.id_rwi = manager.RegisterObject(
            self.render_window.GetInteractor())

        manager.UpdateStatesFromObjects()
        active_ids = manager.GetAllDependencies(0)

        states = map(manager.GetState, active_ids)
        hash_to_blob_map = {blob_hash: manager.GetBlob(
            blob_hash) for blob_hash in manager.GetBlobHashes(active_ids)}
        return states, hash_to_blob_map

    def deserialize(self, states, hash_to_blob_map):

        manager = vtkObjectManager()
        manager.Initialize()
        for state in states:
            manager.RegisterState(state)
        for hash_text, blob in hash_to_blob_map.items():
            manager.RegisterBlob(hash_text, blob)

        manager.UpdateObjectsFromStates()
        active_ids = manager.GetAllDependencies(0)
        manager.GetObjectAtId(self.id_rwi).Render()

    def test(self):
        self.deserialize(*self.serialize())


if __name__ == "__main__":
    vtkLogger.Init()
    # vtkLogger.SetStderrVerbosity(vtkLogger.VERBOSITY_MAX)
    vtkPyTesting.main([(TestSkybox, 'test')])
