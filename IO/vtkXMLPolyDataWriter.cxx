/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPolyDataWriter.cxx
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
#include "vtkXMLPolyDataWriter.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkXMLPolyDataWriter, "1.1");
vtkStandardNewMacro(vtkXMLPolyDataWriter);

//----------------------------------------------------------------------------
vtkXMLPolyDataWriter::vtkXMLPolyDataWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLPolyDataWriter::~vtkXMLPolyDataWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataWriter::SetInput(vtkPolyData* input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkXMLPolyDataWriter::GetInput()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkPolyData*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
const char* vtkXMLPolyDataWriter::GetDataSetName()
{
  return "PolyData";
}

//----------------------------------------------------------------------------
const char* vtkXMLPolyDataWriter::GetDefaultFileExtension()
{
  return "vtp";
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataWriter::SetInputUpdateExtent(int piece, int numPieces,
                                                int ghostLevel)
{
  this->GetInput()->SetUpdateExtent(piece, numPieces, ghostLevel);
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataWriter::WriteInlinePieceAttributes()
{
  this->Superclass::WriteInlinePieceAttributes();
  vtkPolyData* input = this->GetInput();
  this->WriteScalarAttribute("NumberOfVerts",
                             input->GetVerts()->GetNumberOfCells());
  this->WriteScalarAttribute("NumberOfLines",
                             input->GetLines()->GetNumberOfCells());
  this->WriteScalarAttribute("NumberOfStrips",
                             input->GetStrips()->GetNumberOfCells());
  this->WriteScalarAttribute("NumberOfPolys",
                             input->GetPolys()->GetNumberOfCells());
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataWriter::WriteInlinePiece(vtkIndent indent)
{
  this->Superclass::WriteInlinePiece(indent);
  vtkPolyData* input = this->GetInput();
  this->WriteCellsInline("Verts", input->GetVerts(), 0, indent);
  this->WriteCellsInline("Lines", input->GetLines(), 0, indent);
  this->WriteCellsInline("Strips", input->GetStrips(), 0, indent);
  this->WriteCellsInline("Polys", input->GetPolys(), 0, indent);
}  

//----------------------------------------------------------------------------
void vtkXMLPolyDataWriter::WriteAppendedMode(vtkIndent indent)
{  
  this->NumberOfVertsPositions = new unsigned long[this->NumberOfPieces];
  this->NumberOfLinesPositions = new unsigned long[this->NumberOfPieces];
  this->NumberOfStripsPositions = new unsigned long[this->NumberOfPieces];
  this->NumberOfPolysPositions = new unsigned long[this->NumberOfPieces];
  
  this->VertsPositions = new unsigned long*[this->NumberOfPieces];
  this->LinesPositions = new unsigned long*[this->NumberOfPieces];
  this->StripsPositions = new unsigned long*[this->NumberOfPieces];
  this->PolysPositions = new unsigned long*[this->NumberOfPieces];
  
  this->Superclass::WriteAppendedMode(indent);
  
  delete [] this->PolysPositions;
  delete [] this->StripsPositions;
  delete [] this->LinesPositions;
  delete [] this->VertsPositions;
  delete [] this->NumberOfPolysPositions;
  delete [] this->NumberOfStripsPositions;
  delete [] this->NumberOfLinesPositions;
  delete [] this->NumberOfVertsPositions;
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataWriter::WriteAppendedPieceAttributes(int index)
{
  this->Superclass::WriteAppendedPieceAttributes(index);
  this->NumberOfVertsPositions[index] =
    this->ReserveAttributeSpace("NumberOfVerts");
  this->NumberOfLinesPositions[index] =
    this->ReserveAttributeSpace("NumberOfLines");
  this->NumberOfStripsPositions[index] =
    this->ReserveAttributeSpace("NumberOfStrips");
  this->NumberOfPolysPositions[index] =
    this->ReserveAttributeSpace("NumberOfPolys");
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataWriter::WriteAppendedPiece(int index, vtkIndent indent)
{
  this->Superclass::WriteAppendedPiece(index, indent);
  
  this->VertsPositions[index] =
    this->WriteCellsAppended("Verts", 0, indent);
  
  this->LinesPositions[index] =
    this->WriteCellsAppended("Lines", 0, indent);
  
  this->StripsPositions[index] =
    this->WriteCellsAppended("Strips", 0, indent);
  
  this->PolysPositions[index] =
    this->WriteCellsAppended("Polys", 0, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataWriter::WriteAppendedPieceData(int index)
{
  ostream& os = *(this->Stream);
  vtkPolyData* input = this->GetInput();
  
  unsigned long returnPosition = os.tellp();
  os.seekp(this->NumberOfVertsPositions[index]);
  this->WriteScalarAttribute("NumberOfVerts",
                             input->GetVerts()->GetNumberOfCells());
  os.seekp(this->NumberOfLinesPositions[index]);
  this->WriteScalarAttribute("NumberOfLines",
                             input->GetLines()->GetNumberOfCells());
  os.seekp(this->NumberOfStripsPositions[index]);
  this->WriteScalarAttribute("NumberOfStrips",
                             input->GetStrips()->GetNumberOfCells());
  os.seekp(this->NumberOfPolysPositions[index]);
  this->WriteScalarAttribute("NumberOfPolys",
                             input->GetPolys()->GetNumberOfCells());
  os.seekp(returnPosition);
  
  this->Superclass::WriteAppendedPieceData(index);
  
  this->WriteCellsAppendedData(input->GetVerts(), 0,
                               this->VertsPositions[index]);
  this->WriteCellsAppendedData(input->GetLines(), 0,
                               this->LinesPositions[index]);
  this->WriteCellsAppendedData(input->GetStrips(), 0,
                               this->StripsPositions[index]);
  this->WriteCellsAppendedData(input->GetPolys(), 0,
                               this->PolysPositions[index]);
}
