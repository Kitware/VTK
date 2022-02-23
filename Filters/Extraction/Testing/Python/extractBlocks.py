#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()


def GetSelection(ids, inverse=False):
    selectionSource = vtk.vtkSelectionSource()
    selectionSource.SetInverse(inverse)
    selectionSource.SetContentType(vtk.vtkSelectionNode.BLOCKS)
    for i in range(len(ids)):
        selectionSource.AddBlock(ids[i])
    return selectionSource


def GetNumberOfNonNullLeaves(cd):
    count = 0
    it = cd.NewIterator()
    while not it.IsDoneWithTraversal():
        if it.GetCurrentDataObject():
            count += 1
        it.GoToNextItem()
    return count


reader = vtk.vtkExodusIIReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/can.ex2")
reader.UpdateInformation()

extractor = vtk.vtkExtractSelection()
extractor.SetInputConnection(0, reader.GetOutputPort())

# extract 0
selectionSource = GetSelection([0])
extractor.SetInputConnection(1, selectionSource.GetOutputPort())
extractor.Update()
assert GetNumberOfNonNullLeaves(extractor.GetOutputDataObject(0)) == 2

# extract inverse of root
selectionSource = GetSelection([0], True)
extractor.SetInputConnection(1, selectionSource.GetOutputPort())
extractor.Update()
assert GetNumberOfNonNullLeaves(extractor.GetOutputDataObject(0)) == 0

# extract a non-leaf block
selectionSource = GetSelection([1])
extractor.SetInputConnection(1, selectionSource.GetOutputPort())
extractor.Update()
assert GetNumberOfNonNullLeaves(extractor.GetOutputDataObject(0)) == 2

# extract a leaf block
selectionSource = GetSelection([2])
extractor.SetInputConnection(1, selectionSource.GetOutputPort())
extractor.Update()
assert GetNumberOfNonNullLeaves(extractor.GetOutputDataObject(0)) == 1

# extract a non-leaf and leaf block
selectionSource = GetSelection([1, 3])
extractor.SetInputConnection(1, selectionSource.GetOutputPort())
extractor.Update()
assert GetNumberOfNonNullLeaves(extractor.GetOutputDataObject(0)) == 2
