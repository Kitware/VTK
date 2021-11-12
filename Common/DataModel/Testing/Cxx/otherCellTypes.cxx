/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherCellTypes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// this program tests the CellTypes

#include "vtkDebugLeaks.h"

#include "vtkCellType.h"
#include "vtkCellTypes.h"
#include "vtkIntArray.h"

void TestOCT()
{
  // actual test
  vtkCellTypes* ct = vtkCellTypes::New();
  ct->Allocate();

  ct->InsertCell(0, VTK_QUAD, 0);
  ct->InsertNextCell(VTK_PIXEL, 1);

  vtkUnsignedCharArray* cellTypes = vtkUnsignedCharArray::New();

  cellTypes->InsertNextValue(VTK_QUAD);

  cellTypes->InsertNextValue(VTK_PIXEL);

  cellTypes->InsertNextValue(VTK_TETRA);

  ct->SetCellTypes(3, cellTypes);

  ct->DeleteCell(1);

  ct->GetNumberOfTypes();

  ct->IsType(VTK_QUAD);
  ct->IsType(VTK_WEDGE);

  ct->InsertNextType(VTK_WEDGE);
  ct->IsType(VTK_WEDGE);

  ct->GetCellType(2);

  ct->GetActualMemorySize();

  vtkCellTypes* ct1 = vtkCellTypes::New();
  ct1->DeepCopy(ct);

  ct->Reset();
  ct->Squeeze();

  ct1->Delete();
  ct->Delete();
  cellTypes->Delete();
}

int otherCellTypes(int, char*[])
{
  TestOCT();

  // Might need to be adjusted if vtkCellTypes changes
  bool fail1 = (VTK_NUMBER_OF_CELL_TYPES <= VTK_HIGHER_ORDER_HEXAHEDRON);

  // vtkUnstructuredGrid uses uchar to store cellId
  bool fail2 = (VTK_NUMBER_OF_CELL_TYPES > 255);

  return (fail1 || fail2);
}
