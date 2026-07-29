#!/usr/bin/env python
import os.path

from vtkmodules.vtkCommonCore import vtkDoubleArray
from vtkmodules.vtkCommonDataModel import (
    vtkImageData,
    vtkPartitionedDataSet,
    vtkPartitionedDataSetCollection,
    vtkCompositeDataSet
)
from vtkmodules.vtkIOFides import vtkFidesReader, vtkFidesWriter
from vtkmodules.util.misc import vtkGetTempDir

from vtk.test import Testing

VTK_TEMP_DIR = vtkGetTempDir()


def create_dataset(field_name):
    # Return a vtkImageData with one named point array
    img = vtkImageData()
    img.SetDimensions(2, 2, 2)
    arr = vtkDoubleArray()
    arr.SetName(field_name)
    arr.SetNumberOfTuples(8)
    for i in range(8):
        arr.SetValue(i, float(i))
    img.GetPointData().AddArray(arr)
    return img


class TestFidesCollectionFields(Testing.vtkTest):
    # Test reading a Fides partitioned-dataset-collection where each partition
    # has differently named fields.
    #
    # Write two partitions to one Fides file, one with a point field "alpha",
    # the other with a point field "beta". Then make sure that when the collection
    # is read back in, each partition still has the expected fields.
    def testDistinctFieldNamesPerBlock(self):
        pdc = vtkPartitionedDataSetCollection()
        for i, (name, field) in enumerate([("blockA", "alpha"), ("blockB", "beta")]):
            pds = vtkPartitionedDataSet()
            pds.SetPartition(0, create_dataset(field))
            pdc.SetPartitionedDataSet(i, pds)
            pdc.GetMetaData(i).Set(vtkCompositeDataSet.NAME(), name)

        fname = os.path.join(VTK_TEMP_DIR, "TestFidesCollectionFields.bp")
        writer = vtkFidesWriter()
        writer.SetFileName(fname)
        writer.SetInputData(pdc)
        writer.Write()

        reader = vtkFidesReader()
        reader.SetFileName(fname)
        reader.Update()
        out = reader.GetOutputDataObject(0)

        point_arrays = {}
        for i in range(out.GetNumberOfPartitionedDataSets()):
            name = out.GetMetaData(i).Get(vtkCompositeDataSet.NAME())
            ds = out.GetPartitionedDataSet(i).GetPartitionAsDataObject(0)
            pd = ds.GetPointData()
            point_arrays[name] = [pd.GetArrayName(k) for k in range(pd.GetNumberOfArrays())]

        # Make sure each partition's point array kept the expected name
        self.assertEqual(point_arrays.get("blockA"), ["alpha"])
        self.assertEqual(point_arrays.get("blockB"), ["beta"])


if __name__ == "__main__":
    Testing.main([(TestFidesCollectionFields, 'test')])
