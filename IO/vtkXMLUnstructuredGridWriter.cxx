/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredGridWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLUnstructuredGridWriter.h"

#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkXMLUnstructuredGridWriter, "1.2.2.1");
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
  vtkUnstructuredGrid* input = this->GetInput();
  this->WriteScalarAttribute("NumberOfCells", input->GetNumberOfCells());
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteInlinePiece(vtkIndent indent)
{
  vtkUnstructuredGrid* input = this->GetInput();
  this->Superclass::WriteInlinePiece(indent);
  this->WriteCellsInline("Cells", input->GetCells(),
                         input->GetCellTypesArray(), indent);
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteAppendedMode(vtkIndent indent)
{
  this->NumberOfCellsPositions = new unsigned long[this->NumberOfPieces];
  this->CellsPositions = new unsigned long*[this->NumberOfPieces];
  this->Superclass::WriteAppendedMode(indent);
  delete [] this->CellsPositions;
  delete [] this->NumberOfCellsPositions;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteAppendedPieceAttributes(int index)
{
  this->Superclass::WriteAppendedPieceAttributes(index);
  this->NumberOfCellsPositions[index] =
    this->ReserveAttributeSpace("NumberOfCells");
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredGridWriter::WriteAppendedPiece(int index,
                                                      vtkIndent indent)
{
  vtkUnstructuredGrid* input = this->GetInput();
  this->Superclass::WriteAppendedPiece(index, indent);
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
  os.seekp(returnPosition);
  
  this->Superclass::WriteAppendedPieceData(index);
  
  this->WriteCellsAppendedData(input->GetCells(), input->GetCellTypesArray(),
                               this->CellsPositions[index]);
}
