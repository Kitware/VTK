#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()


def GetSelection(ids, inverse=False):
    selNode = vtk.vtkSelectionNode()
    selNode.SetContentType(vtk.vtkSelectionNode.BLOCKS)

    idArray = vtk.vtkIdTypeArray()
    idArray.SetNumberOfTuples(len(ids))
    for i in range(len(ids)):
        idArray.SetValue(i, ids[i])
    selNode.SetSelectionList(idArray)

    if inverse:
        selNode.GetProperties().Set(vtk.vtkSelectionNode.INVERSE(), 1)

    sel = vtk.vtkSelection()
    sel.AddNode(selNode)

    return sel

def GetNumberOfNonNullLeaves(cd):
    count = 0
    it = cd.NewIterator()
    while not it.IsDoneWithTraversal():
        if it.GetCurrentDataObject():
            count+=1
        it.GoToNextItem()
    return count

reader = vtk.vtkExodusIIReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/can.ex2")
reader.UpdateInformation()

extractor = vtk.vtkExtractSelectedBlock()
extractor.SetInputConnection(0, reader.GetOutputPort())

# extract 0
extractor.SetInputDataObject(1, GetSelection([0]))
extractor.Update()
assert GetNumberOfNonNullLeaves(extractor.GetOutputDataObject(0)) == 2

# extract inverse of root
extractor.SetInputDataObject(1, GetSelection([0], True))
extractor.Update()
assert GetNumberOfNonNullLeaves(extractor.GetOutputDataObject(0)) == 0

# extract a non-leaf block
extractor.SetInputDataObject(1, GetSelection([1]))
extractor.Update()
assert GetNumberOfNonNullLeaves(extractor.GetOutputDataObject(0)) == 2

# extract a leaf block
extractor.SetInputDataObject(1, GetSelection([2]))
extractor.Update()
assert GetNumberOfNonNullLeaves(extractor.GetOutputDataObject(0)) == 1


# extract a non-leaf and leaf block
extractor.SetInputDataObject(1, GetSelection([1,3]))
extractor.Update()
assert GetNumberOfNonNullLeaves(extractor.GetOutputDataObject(0)) == 2
