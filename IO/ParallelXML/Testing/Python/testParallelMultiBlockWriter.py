from __future__ import print_function
import shutil, os

import vtk

from vtk.test import Testing
from vtk.util.misc import vtkGetTempDir
VTK_TEMP_DIR = vtkGetTempDir()

contr = vtk.vtkMultiProcessController.GetGlobalController()
if not contr:
    nranks = 1
    rank = 0
else:
    nranks = contr.GetNumberOfProcesses()
    rank = contr.GetLocalProcessId()

# Let's create a simple multiblock with non-empty blocks as indicated below.
# <MB> is a multiblock dataset.
# [<num>] is a leaf node where the number indicates the ranks on which it is
# non-null
# (<num>) is a leaf node where the number indicates the ranks on which it is
# non-empty

# <MB>
#   |
#   |-- <MB>    # level where a block is non-null on only 1 rank
#   |   |
#   |   |-- [0](0)
#   |   |-- [1](1)
#   |   |-- [2](2)
#   |   ... (up to total number of ranks)
#   |
#   |-- <MB>    # level where block is non-null on all ranks, but non-empty only on 1
#   |   |
#   |   |-- [0,1,..nranks](0)
#   |   |-- [0,1,..nranks](1)
#   |   |-- [0,1,..nranks](2)
#   |   ... (up to total number of ranks)
#   |
#   |-- <MB>    # level where block is non-null, non-empty on all ranks.
#   |   |
#   |   |-- [0,1,..nranks](0,1,...nranks)
#   |   |-- [0,1,..nranks](0,1,...nranks)
#   |   |-- [0,1,..nranks](0,1,...nranks)
#   |   ... (up to total number of ranks)


def createDataSet(empty):
    if not empty:
        s = vtk.vtkSphereSource()
        s.Update()
        clone = s.GetOutputDataObject(0).NewInstance()
        clone.ShallowCopy(s.GetOutputDataObject(0))
        return clone
    else:
        return vtk.vtkPolyData()

def createData(non_null_ranks, non_empty_ranks, num_ranks):
    mb = vtk.vtkMultiBlockDataSet()
    mb.SetNumberOfBlocks(num_ranks)
    for i in non_null_ranks:
        mb.SetBlock(i, createDataSet(i not in non_empty_ranks))
    return mb

def createMB(piece, num_pieces):
    output = vtk.vtkMultiBlockDataSet()
    output.SetNumberOfBlocks(3)
    output.SetBlock(0, createData([piece], [piece], num_pieces))
    output.SetBlock(1, createData(range(num_pieces), [piece], num_pieces))
    output.SetBlock(2, createData(range(num_pieces), range(num_pieces), num_pieces))
    return output

writer = vtk.vtkXMLPMultiBlockDataWriter()
prefix =VTK_TEMP_DIR + "/testParallelXMLWriters"
fname = prefix + ".vtm"

if rank == 0:
    try:
        os.remove(fname)
    except OSError: pass
    try:
        shutil.rmtree(prefix)
    except OSError: pass
    print("Output: %s" % fname)

# put a barrier here to make sure that the files are deleted by process 0
# before going further with the test
if contr:
    contr.Barrier()

writer.SetFileName(fname)
writer.SetInputDataObject(createMB(rank, nranks))
writer.Write()

if contr:
    contr.Barrier()

if rank == 0:
    reader = vtk.vtkXMLMultiBlockDataReader()
    reader.SetFileName(fname)
    reader.Update()

    # since verifying the md structure won't reveal if we have written the write
    # set of files, let's just look at the files we wrote out.
    files = os.listdir(prefix)

    expected_file_count = nranks + nranks + (nranks*nranks)
    print ("Expecting %d files for the leaf nodes" % expected_file_count)
    assert (len(files) == expected_file_count)
