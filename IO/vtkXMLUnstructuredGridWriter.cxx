/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLUnstructuredGridWriter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkErrorCode.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkXMLUnstructuredGridWriter, "1.6");
vtkStandardNewMacro(vtkXMLUnstructuredGridWriter);

//----------------------------------------------------------------------------
vtkXMLUnstructuredGridWriter::vtkXMLUnstructuredGridWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLUnstructuredGridWriter::~vtkXMLUnstructuredGridWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::SetInput(vtkUnstructuredGrid* input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLUnstructuredGridWriter::GetInput()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkUnstructuredGrid*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
const char* vtkXMLUnstructuredGridWriter::GetDataSetName()
{
  return "UnstructuredGrid";
}

//----------------------------------------------------------------------------
const char* vtkXMLUnstructuredGridWriter::GetDefaultFileExtension()
{
  return "vtu";
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::SetInputUpdateExtent(int piece,
                                                        int numPieces,
                                                        int ghostLevel)
{
  this->GetInput()->SetUpdateExtent(piece, numPieces, ghostLevel);
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteInlinePieceAttributes()
{
  this->Superclass::WriteInlinePieceAttributes();
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  
  vtkUnstructuredGrid* input = this->GetInput();
  this->WriteScalarAttribute("NumberOfCells", input->GetNumberOfCells());
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteInlinePiece(vtkIndent indent)
{
  vtkUnstructuredGrid* input = this->GetInput();
  
  // Split progress range by the approximate fraction of data written
  // by each step in this method.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[3];
  this->CalculateSuperclassFraction(fractions);
  
  // Set the range of progress for superclass.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Let the superclass write its data.
  this->Superclass::WriteInlinePiece(indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  
  // Set range of progress for the cell specifications.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the cell specifications.
  this->WriteCellsInline("Cells", input->GetCells(),
                         input->GetCellTypesArray(), indent);
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredGridWriter::WriteAppendedMode(vtkIndent indent)
{
  this->NumberOfCellsPositions = new unsigned long[this->NumberOfPieces];
  this->CellsPositions = new unsigned long*[this->NumberOfPieces];
  int ret = this->Superclass::WriteAppendedMode(indent);
  delete [] this->CellsPositions;
  delete [] this->NumberOfCellsPositions;
  
  return ret;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteAppendedPieceAttributes(int index)
{
  this->Superclass::WriteAppendedPieceAttributes(index);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  
  this->NumberOfCellsPositions[index] =
    this->ReserveAttributeSpace("NumberOfCells");
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteAppendedPiece(int index,
                                                      vtkIndent indent)
{
  vtkUnstructuredGrid* input = this->GetInput();
  this->Superclass::WriteAppendedPiece(index, indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  
  this->CellsPositions[index] =
    this->WriteCellsAppended("Cells", input->GetCellTypesArray(), indent);  
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteAppendedPieceData(int index)
{
  ostream& os = *(this->Stream);
  vtkUnstructuredGrid* input = this->GetInput();  
  unsigned long returnPosition = os.tellp();
  os.seekp(this->NumberOfCellsPositions[index]);
  this->WriteScalarAttribute("NumberOfCells", input->GetNumberOfCells());
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  os.seekp(returnPosition);
  
  // Split progress range by the approximate fraction of data written
  // by each step in this method.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[3];
  this->CalculateSuperclassFraction(fractions);
  
  // Set the range of progress for superclass.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Let the superclass write its data.
  this->Superclass::WriteAppendedPieceData(index);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  
  // Set range of progress for the cell specifications.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the cell specifications.
  this->WriteCellsAppendedData(input->GetCells(), input->GetCellTypesArray(),
                               this->CellsPositions[index]);
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredGridWriter::GetNumberOfInputCells()
{
  return this->GetInput()->GetNumberOfCells();
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::CalculateSuperclassFraction(float* fractions)
{
  vtkUnstructuredGrid* input = this->GetInput();
  
  // The superclass will write point/cell data and point specifications.
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  vtkIdType pdSize = pdArrays*this->GetNumberOfInputPoints();
  vtkIdType cdSize = cdArrays*this->GetNumberOfInputCells();
  vtkIdType pointsSize = this->GetNumberOfInputPoints();
  
  // This class will write cell specifications.
  vtkIdType connectSize = (input->GetCells()->GetData()->GetNumberOfTuples() -
                           input->GetNumberOfCells());
  vtkIdType offsetSize = input->GetNumberOfCells();
  vtkIdType typesSize = input->GetNumberOfCells();
  int total = (pdSize+cdSize+pointsSize+connectSize+offsetSize+typesSize);
  if(total == 0)
    {
    total = 1;
    }
  fractions[0] = 0;
  fractions[1] = float(pdSize+cdSize+pointsSize)/total;
  fractions[2] = 1;
}
