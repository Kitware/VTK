from vtkmodules.vtkIOParallelXML import vtkXMLPartitionedDataSetCollectionWriter
from vtkmodules.vtkIOXML import vtkXMLPartitionedDataSetCollectionReader
from vtkmodules.vtkFiltersSources import vtkPartitionedDataSetCollectionSource

from vtk.test import Testing
from vtk.util.misc import vtkGetTempDir

import os, shutil

class TestXMLPartitionedDataSetCollection(Testing.vtkTest):

    def test(self):
        tmpdir = vtkGetTempDir()
        fname = tmpdir + "/testxmlpartitioneddatasetcollection.vtpc"

        source = vtkPartitionedDataSetCollectionSource()
        writer = vtkXMLPartitionedDataSetCollectionWriter()
        writer.SetInputConnection(source.GetOutputPort())
        writer.SetFileName(fname)
        writer.Write()

        reader = vtkXMLPartitionedDataSetCollectionReader()
        reader.SetFileName(fname)
        reader.Update()

        validData = source.GetOutputDataObject(0)
        data = reader.GetOutputDataObject(0)

        assert data.IsA("vtkPartitionedDataSetCollection")
        assert validData.GetNumberOfPartitionedDataSets() == data.GetNumberOfPartitionedDataSets()
        for i in range(validData.GetNumberOfPartitionedDataSets()):
            assert validData.GetNumberOfPartitions(i) == data.GetNumberOfPartitions(i)
            assert validData.GetMetaData(i).Get(validData.NAME()) == data.GetMetaData(i).Get(data.NAME())
        assert validData.GetDataAssembly() is not None and data.GetDataAssembly() is not None

        os.remove(fname)
        shutil.rmtree(tmpdir + "/testxmlpartitioneddatasetcollection")

if __name__ == "__main__":
    Testing.main([(TestXMLPartitionedDataSetCollection, 'test')])
