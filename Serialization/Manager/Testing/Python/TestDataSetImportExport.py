from vtkmodules.test import Testing as vtkTesting
from vtkmodules.vtkSerializationManager import vtkObjectManager
from vtkmodules.vtkCommonCore import vtkObjectBase, vtkCharArray, vtkIndent
from vtkmodules.vtkCommonDataModel import vtkImageData, vtkDataObjectTreeIterator
from vtkmodules.vtkFiltersSources import vtkSphereSource, vtkCellTypeSource, vtkPartitionedDataSetCollectionSource


def create_test_datasets():
    # Create a vtkPolyData (Sphere)
    sphere_source = vtkSphereSource()
    sphere_source.Update()
    poly_data = sphere_source.GetOutput()

    # Create a vtkUnstructuredGrid (Cell Type Source)
    cell_type_source = vtkCellTypeSource()
    cell_type_source.Update()
    unstructured_grid = cell_type_source.GetOutput()

    # Create a vtkImageData
    image_data = vtkImageData()
    image_data.SetDimensions(10, 10, 10)

    # Create a vtkPartitionedDataSetCollection
    pds_collection_source = vtkPartitionedDataSetCollectionSource()
    pds_collection_source.Update()
    pds_collection = pds_collection_source.GetOutput()

    return poly_data, unstructured_grid, image_data, pds_collection


def serialize(vtk_object: vtkObjectBase) -> vtkCharArray:
    manager = vtkObjectManager()
    manager.Initialize()
    manager.RegisterObject(vtk_object)
    manager.UpdateStatesFromObjects()
    # Equivalent to when using files:
    # manager.Export("data_object")
    return manager.ExportToBytes()


def deserialize(serialized_data: vtkCharArray) -> vtkObjectBase:
    manager = vtkObjectManager()
    manager.Initialize()
    # return value is a list of strong objects
    strong_objects = manager.ImportFromBytes(serialized_data)
    # Return the first object which is the deserialized object
    return strong_objects[0]
    # Equivalent to when using files:
    # manager.Import("data_object.states.json", "data_object.blobs.json")
    # strong_object = manager.GetObjectAtId(1)  # Assuming the first object has ID 1
    # return strong_object


class TestDataSetImportExport(vtkTesting.vtkTest):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.poly_data, self.unstructured_grid, self.image_data, self.pds_collection = create_test_datasets()

    def test_polydata(self):
        serialized_poly_data = serialize(self.poly_data)
        deserialized_poly_data = deserialize(serialized_poly_data)
        self.assertTrue(deserialized_poly_data.IsA("vtkPolyData"))
        self.assertEqual(self.poly_data.GetNumberOfPoints(),
                         deserialized_poly_data.GetNumberOfPoints())
        self.assertEqual(self.poly_data.GetNumberOfCells(),
                         deserialized_poly_data.GetNumberOfCells())
        self.assertTupleEqual(self.poly_data.GetBounds(),
                              deserialized_poly_data.GetBounds())
        for dataset_attrs, serialized_attrs in zip([self.poly_data.point_data, self.poly_data.cell_data, self.poly_data.field_data], [deserialized_poly_data.point_data, deserialized_poly_data.cell_data, deserialized_poly_data.field_data]):
            for array_name in [dataset_attrs.GetArrayName(i) for i in range(dataset_attrs.GetNumberOfArrays())]:
                original_array = dataset_attrs.GetArray(array_name)
                deserialized_array = serialized_attrs.GetArray(array_name)
                self.assertTupleEqual(
                    original_array.GetRange(), deserialized_array.GetRange())

    def test_unstructured_grid(self):
        serialized_unstructured_grid = serialize(self.unstructured_grid)
        deserialized_unstructured_grid = deserialize(
            serialized_unstructured_grid)
        self.assertTrue(deserialized_unstructured_grid.IsA(
            "vtkUnstructuredGrid"))
        self.assertEqual(self.unstructured_grid.GetNumberOfPoints(
        ), deserialized_unstructured_grid.GetNumberOfPoints())
        self.assertEqual(self.unstructured_grid.GetNumberOfCells(
        ), deserialized_unstructured_grid.GetNumberOfCells())
        self.assertTupleEqual(self.unstructured_grid.GetBounds(
        ), deserialized_unstructured_grid.GetBounds())
        for dataset_attrs, serialized_attrs in zip([self.unstructured_grid.point_data, self.unstructured_grid.cell_data, self.unstructured_grid.field_data], [deserialized_unstructured_grid.point_data, deserialized_unstructured_grid.cell_data, deserialized_unstructured_grid.field_data]):
            for array_name in [dataset_attrs.GetArrayName(i) for i in range(dataset_attrs.GetNumberOfArrays())]:
                original_array = dataset_attrs.GetArray(array_name)
                deserialized_array = serialized_attrs.GetArray(array_name)
                self.assertTupleEqual(
                    original_array.GetRange(), deserialized_array.GetRange())

    def test_image_data(self):
        serialized_image_data = serialize(self.image_data)
        deserialized_image_data = deserialize(serialized_image_data)
        self.assertTrue(deserialized_image_data.IsA("vtkImageData"))
        self.assertEqual(self.image_data.GetDimensions(),
                         deserialized_image_data.GetDimensions())
        self.assertTupleEqual(self.image_data.GetBounds(),
                              deserialized_image_data.GetBounds())
        for dataset_attrs, serialized_attrs in zip([self.image_data.point_data, self.image_data.cell_data, self.image_data.field_data], [deserialized_image_data.point_data, deserialized_image_data.cell_data, deserialized_image_data.field_data]):
            for array_name in [dataset_attrs.GetArrayName(i) for i in range(dataset_attrs.GetNumberOfArrays())]:
                original_array = dataset_attrs.GetArray(array_name)
                deserialized_array = serialized_attrs.GetArray(array_name)
                self.assertTupleEqual(
                    original_array.GetRange(), deserialized_array.GetRange())

    def test_partitioned_data_set_collection(self):
        serialized_pds_collection = serialize(self.pds_collection)
        deserialized_pds_collection = deserialize(serialized_pds_collection)
        self.assertTrue(deserialized_pds_collection.IsA(
            "vtkPartitionedDataSetCollection"))
        self.assertEqual(self.pds_collection.GetNumberOfPartitionedDataSets(
        ), deserialized_pds_collection.GetNumberOfPartitionedDataSets())
        for i in range(self.pds_collection.GetNumberOfPartitionedDataSets()):
            self.assertEqual(self.pds_collection.GetNumberOfPartitions(
                i), deserialized_pds_collection.GetNumberOfPartitions(i))
        deserialized_block_iterator = vtkDataObjectTreeIterator(
            data_set=deserialized_pds_collection, visit_only_leaves=True, traverse_sub_tree=True)
        deserialized_block_iterator.InitTraversal()
        original_block_iterator = vtkDataObjectTreeIterator(
            data_set=self.pds_collection, visit_only_leaves=True, traverse_sub_tree=True)
        original_block_iterator.InitTraversal()
        self.assertEqual(self.pds_collection.data_assembly.SerializeToXML(
            vtkIndent()), deserialized_pds_collection.data_assembly.SerializeToXML(vtkIndent()))
        while not (deserialized_block_iterator.IsDoneWithTraversal() and original_block_iterator.IsDoneWithTraversal()):
            original_block = original_block_iterator.GetCurrentDataObject()
            deserialized_block = deserialized_block_iterator.GetCurrentDataObject()
            self.assertEqual(original_block.GetNumberOfPoints(),
                             deserialized_block.GetNumberOfPoints())
            self.assertEqual(original_block.GetNumberOfCells(),
                             deserialized_block.GetNumberOfCells())
            self.assertTupleEqual(original_block.GetBounds(),
                                  deserialized_block.GetBounds())
            for dataset_attrs, serialized_attrs in zip([original_block.point_data, original_block.cell_data, original_block.field_data], [deserialized_block.point_data, deserialized_block.cell_data, deserialized_block.field_data]):
                for array_name in [dataset_attrs.GetArrayName(i) for i in range(dataset_attrs.GetNumberOfArrays())]:
                    original_array = dataset_attrs.GetArray(array_name)
                    deserialized_array = serialized_attrs.GetArray(array_name)
                    self.assertTupleEqual(
                        original_array.GetRange(), deserialized_array.GetRange())
            deserialized_block_iterator.GoToNextItem()
            original_block_iterator.GoToNextItem()


if __name__ == "__main__":
    vtkTesting.main([(TestDataSetImportExport, 'test_polydata'),
                     (TestDataSetImportExport, 'test_unstructured_grid'),
                     (TestDataSetImportExport, 'test_image_data'),
                     (TestDataSetImportExport, 'test_partitioned_data_set_collection')])
