/*==========================================================================

  Program: 
  Module:    otherCellTypes.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  ==========================================================================*/

// .NAME 
// .SECTION Description
// this program tests the CellTypes

#include "vtkDebugLeaks.h"

#include "vtkCellTypes.h"
#include "vtkCellType.h"

void Test(ostream& strm)
{
  // actual test
  strm << "Test CellTypes Start" << endl;
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
  strm << "Test CellTypes Complete" << endl;
}

int main(int argc, char* argv[])
{
  vtkDebugLeaks::PromptUserOff();

  Test(cout);

  return 0;
} 
