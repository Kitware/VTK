from vtkmodules.test import Testing as vtkPyTesting
from vtkmodules.vtkTestingRendering import vtkTesting as vtkCppTesting
from vtkmodules.vtkSerializationManager import vtkObjectManager
from vtkmodules.vtkCommonCore import vtkFloatArray, vtkLogger
from vtkmodules.vtkCommonDataModel import vtkImageData
from vtkmodules.vtkRenderingCore import vtkTexture
import vtkmodules.vtkRenderingOpenGL2


class TestObjectManagerTexture(vtkPyTesting.vtkTest):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def setUp(self):
        self.id_texture = 0
        self.id_image = 0

        self.serialized_image = vtkImageData()
        self.serialized_image.SetDimensions(2, 2, 2)

        scalars = vtkFloatArray()
        scalars.SetNumberOfComponents(4)
        scalars.SetNumberOfTuples(8)
        scalars.FillComponent(0, 0.2)
        scalars.FillComponent(1, 0.4)
        scalars.FillComponent(2, 0.6)
        scalars.FillComponent(3, 1)
        self.serialized_image.GetPointData().SetScalars(scalars)

        self.deserialized_image = None

    def serialize(self):

        texture = vtkTexture()
        texture.SetInputData(self.serialized_image)
        texture.Update()

        manager = vtkObjectManager()
        manager.Initialize()
        self.id_texture = manager.RegisterObject(texture)

        manager.UpdateStatesFromObjects()
        self.id_image = manager.GetId(self.serialized_image)
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
        self.deserialized_image = manager.GetObjectAtId(self.id_image)

    def test(self):
        self.deserialize(*self.serialize())
        cppTesting = vtkCppTesting()
        self.assertTrue(cppTesting.CompareAverageOfL2Norm(
            self.serialized_image, self.deserialized_image, 0.0))


if __name__ == "__main__":
    vtkLogger.Init()
    # vtkLogger.SetStderrVerbosity(vtkLogger.VERBOSITY_MAX)
    vtkPyTesting.main([(TestObjectManagerTexture, 'test')])
