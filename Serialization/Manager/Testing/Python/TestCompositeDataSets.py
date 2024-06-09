from vtkmodules.test import Testing as vtkTesting
from vtkmodules.vtkCommonCore import vtkIndent
from vtkmodules.vtkCommonDataModel import vtkCompositeDataSet
from vtkmodules.vtkFiltersSources import vtkPartitionedDataSetCollectionSource
from vtkmodules.vtkFiltersCore import vtkConvertToMultiBlockDataSet
from vtkmodules.vtkSerializationManager import vtkObjectManager


class TestCompositeDataSets(vtkTesting.vtkTest):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.exercise_multi_block = True

    def setUp(self):
        self.id_data_object = None

        self.source = vtkPartitionedDataSetCollectionSource()
        self.source.SetNumberOfShapes(12)

    def initialize_data_object(self):
        if self.exercise_multi_block:
            c = vtkConvertToMultiBlockDataSet()
            c.SetInputConnection(self.source.GetOutputPort())
            c.Update()

            self.data_object = c.GetOutput()
        else:
            self.source.Update()
            self.data_object = self.source.GetOutput()

    def serialize(self):

        manager = vtkObjectManager()
        manager.Initialize()
        self.id_data_object = manager.RegisterObject(self.data_object)

        manager.UpdateStatesFromObjects()
        active_ids = manager.GetAllDependencies(0)
        manager.Export("state.json")

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
        self.deserialized_data_object = manager.GetObjectAtId(
            self.id_data_object)

    def test_multiblock_dataset(self):
        self.exercise_multi_block = True
        self.initialize_data_object()

        self.deserialize(*self.serialize())

        for i in range(12):
            self.assertEqual(self.data_object.GetMetaData(i).Get(vtkCompositeDataSet.NAME(
            )), self.deserialized_data_object.GetMetaData(i).Get(vtkCompositeDataSet.NAME()))
            self.assertEqual(self.data_object.GetBlock(i).GetNumberOfPoints(
            ), self.deserialized_data_object.GetBlock(i).GetNumberOfPoints())
            self.assertEqual(self.data_object.GetBlock(i).GetNumberOfCells(
            ), self.deserialized_data_object.GetBlock(i).GetNumberOfCells())

    def test_partitioned_dataset_collection(self):
        self.exercise_multi_block = False
        self.initialize_data_object()

        self.deserialize(*self.serialize())

        for i in range(12):
            self.assertEqual(self.data_object.GetMetaData(i).Get(vtkCompositeDataSet.NAME(
            )), self.deserialized_data_object.GetMetaData(i).Get(vtkCompositeDataSet.NAME()))
            self.assertEqual(self.data_object.GetPartitionedDataSet(i).GetNumberOfPartitions(
            ), self.deserialized_data_object.GetPartitionedDataSet(i).GetNumberOfPartitions())
            for j in range(self.data_object.GetNumberOfPartitions(i)):
                self.assertEqual(
                    self.data_object.GetPartition(i, j).GetNumberOfCells(),
                    self.deserialized_data_object.GetPartition(i, j).GetNumberOfCells())
                self.assertEqual(
                    self.data_object.GetPartition(i, j).GetNumberOfPoints(),
                    self.deserialized_data_object.GetPartition(i, j).GetNumberOfPoints())
                self.assertEqual(
                    self.data_object.GetPartition(
                        i, j).GetPointData().GetNumberOfArrays(),
                    self.deserialized_data_object.GetPartition(i, j).GetPointData().GetNumberOfArrays())

        self.assertEqual(
            self.data_object.GetDataAssembly().SerializeToXML(vtkIndent()),
            self.deserialized_data_object.GetDataAssembly().SerializeToXML(vtkIndent()))


if __name__ == "__main__":
    vtkTesting.main([(TestCompositeDataSets, 'test')])
