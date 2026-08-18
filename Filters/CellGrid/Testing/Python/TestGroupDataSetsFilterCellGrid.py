# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause

from vtkmodules.vtkCommonDataModel import vtkPartitionedDataSetCollection, vtkCellGrid
from vtkmodules.vtkFiltersCellGrid import vtkFiltersCellGrid
from vtkmodules.vtkFiltersGeneral import vtkGroupDataSetsFilter
from vtkmodules.vtkIOCellGrid import vtkCellGridReader
from vtkmodules.test import Testing

import os

# Ensure cell types/responders are registered so the reader produces a cell grid
vtkFiltersCellGrid.RegisterCellsAndResponders()


# Ensure that vtkCellGrid no longer gets dropped from a grouped PDC
class TestGroupDataSetsFilterCellGrid(Testing.vtkTest):
    def readCellGrid(self):
        reader = vtkCellGridReader()
        reader.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg'))
        reader.Update()
        grid = reader.GetOutputDataObject(0)
        self.assertTrue(grid.IsA('vtkCellGrid'), 'Expected a vtkCellGrid from the reader.')
        return grid

    def countCellGridPartitions(self, pdc):
        count = 0
        for i in range(pdc.GetNumberOfPartitionedDataSets()):
            for j in range(pdc.GetNumberOfPartitions(i)):
                if vtkCellGrid.SafeDownCast(pdc.GetPartitionAsDataObject(i, j)):
                    count += 1
        return count

    def testGroupCellGridPartitions(self):
        pdc = vtkPartitionedDataSetCollection()
        pdc.SetNumberOfPartitionedDataSets(1)
        pdc.SetPartition(0, 0, self.readCellGrid())
        self.assertEqual(self.countCellGridPartitions(pdc), 1)

        group = vtkGroupDataSetsFilter()
        group.SetInputDataObject(pdc)
        group.SetOutputTypeToPartitionedDataSetCollection()
        group.Update()
        out = vtkPartitionedDataSetCollection.SafeDownCast(group.GetOutputDataObject(0))

        self.assertEqual(self.countCellGridPartitions(out), 1,
                         'vtkGroupDataSetsFilter dropped the vtkCellGrid partition.')


if __name__ == '__main__':
    Testing.main([(TestGroupDataSetsFilterCellGrid, 'test')])
