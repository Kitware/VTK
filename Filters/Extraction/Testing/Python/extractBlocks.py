#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkSelectionNode
from vtkmodules.vtkFiltersExtraction import vtkExtractSelection
from vtkmodules.vtkFiltersSources import vtkSelectionSource
from vtkmodules.vtkIOExodus import vtkExodusIIReader
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()


def GetSelection(ids, inverse=False):
    selectionSource = vtkSelectionSource()
    selectionSource.SetInverse(inverse)
    selectionSource.SetContentType(vtkSelectionNode.BLOCKS)
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


reader = vtkExodusIIReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/can.ex2")
reader.UpdateInformation()

extractor = vtkExtractSelection()
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
