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

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkXMLPolyDataWriter, "1.3");
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
  // Split progress range by the approximate fraction of data written
  // by each step in this method.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[6];
  this->CalculateSuperclassFraction(fractions);
  
  // Set the range of progress for superclass.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Let the superclass write its data.  
  this->Superclass::WriteInlinePiece(indent);
  
  vtkPolyData* input = this->GetInput();
  
  // Set the range of progress for Verts.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the Verts.
  this->WriteCellsInline("Verts", input->GetVerts(), 0, indent);
  
  // Set the range of progress for Lines.
  this->SetProgressRange(progressRange, 2, fractions);
  
  // Write the Lines.
  this->WriteCellsInline("Lines", input->GetLines(), 0, indent);
  
  // Set the range of progress for Strips.
  this->SetProgressRange(progressRange, 3, fractions);
  
  // Write the Strips.
  this->WriteCellsInline("Strips", input->GetStrips(), 0, indent);
  
  // Set the range of progress for Polys.
  this->SetProgressRange(progressRange, 4, fractions);
  
  // Write the Polys.
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
  
  // Split progress range by the approximate fraction of data written
  // by each step in this method.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[6];
  this->CalculateSuperclassFraction(fractions);
  
  // Set the range of progress for superclass.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Let the superclass write its data.  
  this->Superclass::WriteAppendedPieceData(index);
  
  // Set the range of progress for Verts.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the Verts.
  this->WriteCellsAppendedData(input->GetVerts(), 0,
                               this->VertsPositions[index]);
  
  // Set the range of progress for Lines.
  this->SetProgressRange(progressRange, 2, fractions);
  
  // Write the Lines.
  this->WriteCellsAppendedData(input->GetLines(), 0,
                               this->LinesPositions[index]);
  
  // Set the range of progress for Strips.
  this->SetProgressRange(progressRange, 3, fractions);
  
  // Write the Strips.
  this->WriteCellsAppendedData(input->GetStrips(), 0,
                               this->StripsPositions[index]);
  
  // Set the range of progress for Polys.
  this->SetProgressRange(progressRange, 4, fractions);
  
  // Write the Polys.
  this->WriteCellsAppendedData(input->GetPolys(), 0,
                               this->PolysPositions[index]);
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPolyDataWriter::GetNumberOfInputCells()
{
  vtkPolyData* input = this->GetInput();
  return (input->GetVerts()->GetNumberOfCells()+
          input->GetLines()->GetNumberOfCells()+
          input->GetStrips()->GetNumberOfCells()+
          input->GetPolys()->GetNumberOfCells());
}

//----------------------------------------------------------------------------
void vtkXMLPolyDataWriter::CalculateSuperclassFraction(float* fractions)
{
  vtkPolyData* input = this->GetInput();
  
  // The superclass will write point/cell data and point specifications.
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  vtkIdType pdSize = pdArrays*this->GetNumberOfInputPoints();
  vtkIdType cdSize = cdArrays*this->GetNumberOfInputCells();
  vtkIdType pointsSize = this->GetNumberOfInputPoints();
  
  // This class will write cell specifications.
  vtkIdType connectSizeV = (input->GetVerts()->GetData()->GetNumberOfTuples() -
                            input->GetVerts()->GetNumberOfCells());
  vtkIdType connectSizeL = (input->GetLines()->GetData()->GetNumberOfTuples() -
                            input->GetLines()->GetNumberOfCells());
  vtkIdType connectSizeS = (input->GetStrips()->GetData()->GetNumberOfTuples() -
                            input->GetStrips()->GetNumberOfCells());
  vtkIdType connectSizeP = (input->GetPolys()->GetData()->GetNumberOfTuples() -
                            input->GetPolys()->GetNumberOfCells());
  vtkIdType offsetSizeV = input->GetVerts()->GetNumberOfCells();
  vtkIdType offsetSizeL = input->GetLines()->GetNumberOfCells();
  vtkIdType offsetSizeS = input->GetStrips()->GetNumberOfCells();
  vtkIdType offsetSizeP = input->GetPolys()->GetNumberOfCells();
  fractions[0] = 0;
  fractions[1] = fractions[0] + pdSize+cdSize+pointsSize;
  fractions[2] = fractions[1] + connectSizeV+offsetSizeV;
  fractions[3] = fractions[2] + connectSizeL+offsetSizeL;
  fractions[4] = fractions[3] + connectSizeS+offsetSizeS;
  fractions[5] = fractions[4] + connectSizeP+offsetSizeP;
  if(fractions[5] == 0)
    {
    fractions[5] = 1;
    }
  for(int i=0; i < 5;++i)
    {
    fractions[i+1] = fractions[i+1] / fractions[5];
    }
}
