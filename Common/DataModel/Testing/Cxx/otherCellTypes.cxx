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

#include "vtkCellTypes.h"
#include "vtkCellType.h"

void TestOCT()
{
  // actual test
  vtkCellTypes *ct = vtkCellTypes::New();
  ct->Allocate();

  ct->InsertCell(0, VTK_QUAD, 0);
  ct->InsertNextCell(VTK_PIXEL, 1);

  vtkUnsignedCharArray *cellTypes = vtkUnsignedCharArray::New();
  vtkIntArray *cellLocations = vtkIntArray::New();

  cellLocations->InsertNextValue (0);
  cellTypes->InsertNextValue(VTK_QUAD);

  cellLocations->InsertNextValue (1);
  cellTypes->InsertNextValue(VTK_PIXEL);

  cellLocations->InsertNextValue (2);
  cellTypes->InsertNextValue(VTK_TETRA);

  ct->SetCellTypes (3, cellTypes, cellLocations);

  ct->GetCellLocation (1);
  ct->DeleteCell(1);

  ct->GetNumberOfTypes();

  ct->IsType(VTK_QUAD);
  ct->IsType(VTK_WEDGE);

  ct->InsertNextType(VTK_WEDGE);
  ct->IsType(VTK_WEDGE);

  ct->GetCellType(2);

  ct->GetActualMemorySize();

  vtkCellTypes *ct1 = vtkCellTypes::New();
  ct1->DeepCopy(ct);

  ct->Reset();
  ct->Squeeze();

  ct1->Delete();
  ct->Delete();
  cellLocations->Delete();
  cellTypes->Delete();
}

int otherCellTypes(int, char *[])
{
  TestOCT();

  // Might need to be ajusted if vtkCellTypes changes
  if( VTK_NUMBER_OF_CELL_TYPES <= VTK_HIGHER_ORDER_HEXAHEDRON)
    {
    return 1;
    }
  // vtkUnstructuredGrid uses uchar to store cellId
  if( VTK_NUMBER_OF_CELL_TYPES > 255 )
    {
    return 1;
    }

  return 0;
}
