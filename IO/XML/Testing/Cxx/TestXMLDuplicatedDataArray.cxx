// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLTableReader.h"
#include "vtkXMLUnstructuredGridReader.h"

static const char* testUGDuplicatedArray = R"==(<?xml version="1.0"?>
<VTKFile type="UnstructuredGrid"  version="0.1" >
  <UnstructuredGrid>
    <FieldData>
      <DataArray type="Int32" Name="FieldData" NumberOfTuples="1" format="ascii" RangeMin="1" RangeMax="1">
        4
      </DataArray>
      <DataArray type="Int32" Name="FieldData" NumberOfTuples="1" format="ascii" RangeMin="1" RangeMax="1">
        5
      </DataArray>
    </FieldData>
    <Piece  NumberOfPoints="4" NumberOfCells="1">
      <Points>
        <DataArray  type="Float64"  NumberOfComponents="3"  format="ascii"> 0 0 0  1 0 0  1 1 0  0 1 0  </DataArray>
      </Points>
      <Cells>
        <DataArray  type="Int32"  Name="connectivity"  format="ascii">4 0 1 2 3</DataArray>
        <DataArray  type="Int64"  Name="offsets"  format="ascii"> 0 </DataArray>
        <DataArray  type="UInt8"  Name="types"  format="ascii"> 10 </DataArray>
      </Cells>
      <PointData  Scalars="u">
        <DataArray  type="Float64"  Name="u"  format="ascii"> 1.0 2.0 3.0 4.0 </DataArray>
        <DataArray  type="Float64"  Name="u"  format="ascii"> 5.0 6.0 7.0 8.0 </DataArray>
        <DataArray  type="Float64"  Name="v"  format="ascii"> 9.0 10.0 11.0 12.0 </DataArray>
      </PointData>
      <CellData  Scalars="k">
        <DataArray  type="Float64"  Name="k"  format="ascii"> 1.0 2.0 3.0 4.0 </DataArray>
        <DataArray  type="Float64"  Name="k"  format="ascii"> 5.0 6.0 7.0 8.0 </DataArray>
        <DataArray  type="Float64"  Name="l"  format="ascii"> 9.0 10.0 11.0 12.0 </DataArray>
      </CellData>
    </Piece>
  </UnstructuredGrid>
</VTKFile>
)==";

static const char* testTableDuplicatedArray = R"==(<?xml version="1.0"?>
<VTKFile type="Table" version="1.0" byte_order="LittleEndian" header_type="UInt64">
  <Table>
    <Piece NumberOfCols="2" NumberOfRows="4">
      <RowData>
        <DataArray type="Float32" Name="Elevation" format="ascii">
          1.0 2.0 3.0 4.0
        </DataArray>
        <DataArray type="Float32" Name="Elevation" format="ascii">
          5.0 6.0 7.0 8.0
        </DataArray>
      </RowData>
    </Piece>
  </Table>
</VTKFile>
)==";

namespace
{
int TestUG()
{
  vtkNew<vtkXMLUnstructuredGridReader> readerDuplicated;
  readerDuplicated->ReadFromInputStringOn();
  readerDuplicated->SetInputString(testUGDuplicatedArray);
  readerDuplicated->Update();
  vtkUnstructuredGrid* output = readerDuplicated->GetOutput();

  if (output->GetNumberOfPoints() != 4)
  {
    vtkLogF(ERROR, "Expected %d points, but got %lld.", 4, readerDuplicated->GetNumberOfPoints());
    return EXIT_FAILURE;
  }
  if (output->GetNumberOfCells() != 1)
  {
    vtkLogF(ERROR, "Expected %d cell, but got %lld.", 1, readerDuplicated->GetNumberOfCells());
    return EXIT_FAILURE;
  }

  // Point data
  if (output->GetPointData()->GetNumberOfArrays() != 2)
  {
    vtkLogF(ERROR, "Expected %d point data arrays, but got %d.", 2,
      output->GetPointData()->GetNumberOfArrays());
    return EXIT_FAILURE;
  }

  double val = output->GetPointData()->GetArray("u")->GetTuple1(0);
  if (val != 1.0)
  {
    vtkLogF(ERROR, "Invalid data in array 'u', expected %.1f but got %.1f", 1.0, val);
    return EXIT_FAILURE;
  }

  val = output->GetPointData()->GetArray("v")->GetTuple1(0);
  if (val != 9.0)
  {
    vtkLogF(ERROR, "Invalid data in array 'v', expected %.1f but got %.1f", 9.0, val);
    return EXIT_FAILURE;
  }

  // Cell data
  if (output->GetCellData()->GetNumberOfArrays() != 2)
  {
    vtkLogF(ERROR, "Expected %d cell data arrays, but got %d.", 2,
      output->GetCellData()->GetNumberOfArrays());
    return EXIT_FAILURE;
  }

  val = output->GetCellData()->GetArray("k")->GetTuple1(0);
  if (val != 1.0)
  {
    vtkLogF(ERROR, "Invalid data in array 'k', expected %.1f but got %.1f", 1.0, val);
    return EXIT_FAILURE;
  }

  val = output->GetCellData()->GetArray("l")->GetTuple1(0);
  if (val != 9.0)
  {
    vtkLogF(ERROR, "Invalid data in array 'l', expected %.1f but got %.1f", 9.0, val);
    return EXIT_FAILURE;
  }

  // Field data
  if (output->GetFieldData()->GetNumberOfArrays() != 1)
  {
    vtkLogF(ERROR, "Expected %d field data arrays, but got %d.", 1,
      output->GetFieldData()->GetNumberOfArrays());
    return EXIT_FAILURE;
  }

  val = output->GetFieldData()->GetArray("FieldData")->GetTuple1(0);
  if (val != 4.0)
  {
    vtkLogF(ERROR, "Invalid data in array 'FieldData', expected %.1f but got %.1f", 4.0, val);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int TestTable()
{
  vtkNew<vtkXMLTableReader> tableReader;
  tableReader->ReadFromInputStringOn();
  tableReader->SetInputString(testTableDuplicatedArray);
  tableReader->Update();
  vtkTable* output = tableReader->GetOutput();

  if (output->GetNumberOfColumns() != 1)
  {
    vtkLogF(ERROR, "Expected %d colunms, but got %lld.", 1, output->GetNumberOfColumns());
    return EXIT_FAILURE;
  }

  double val = vtkFloatArray::SafeDownCast(output->GetColumn(0))->GetTuple1(0);
  if (val != 1.0)
  {
    vtkLogF(ERROR, "Invalid data in column, expected %.1f but got %.1f", 1.0, val);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
}

int TestXMLDuplicatedDataArray(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  return ::TestUG() || ::TestTable();
}
