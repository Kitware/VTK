/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLRectilinearGridWriter.cxx
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
#include "vtkXMLRectilinearGridWriter.h"

#include "vtkCellData.h"
#include "vtkExtentTranslator.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"

vtkCxxRevisionMacro(vtkXMLRectilinearGridWriter, "1.4");
vtkStandardNewMacro(vtkXMLRectilinearGridWriter);

//----------------------------------------------------------------------------
vtkXMLRectilinearGridWriter::vtkXMLRectilinearGridWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLRectilinearGridWriter::~vtkXMLRectilinearGridWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridWriter::SetInput(vtkRectilinearGrid* input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkXMLRectilinearGridWriter::GetInput()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkRectilinearGrid*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridWriter::GetInputExtent(int* extent)
{
  this->GetInput()->GetExtent(extent);
}

//----------------------------------------------------------------------------
const char* vtkXMLRectilinearGridWriter::GetDataSetName()
{
  return "RectilinearGrid";
}

//----------------------------------------------------------------------------
const char* vtkXMLRectilinearGridWriter::GetDefaultFileExtension()
{
  return "vtr";
}

//----------------------------------------------------------------------------
vtkDataArray*
vtkXMLRectilinearGridWriter::CreateExactCoordinates(vtkDataArray* a, int xyz)
{
  int inExtent[6];
  int outExtent[6];
  this->GetInput()->GetExtent(inExtent);
  this->ExtentTranslator->GetExtent(outExtent);
  int* inBounds = inExtent+xyz*2;
  int* outBounds = outExtent+xyz*2;
  
  if(!a)
    {
    // There are no coordinates.  This can happen with empty input.
    return vtkFloatArray::New();
    }
  
  if((inBounds[0] == outBounds[0]) && (inBounds[1] == outBounds[1]))
    {
    // Use the entire coordinates array.
    a->Register(0);
    return a;
    }
  else
    {
    // Create a subset of the coordinates array.
    int components = a->GetNumberOfComponents();
    int tupleSize = components*this->GetWordTypeSize(a->GetDataType());
    vtkDataArray* b = a->NewInstance();
    b->SetNumberOfComponents(components);
    b->SetName(a->GetName());
    int tuples = outBounds[1] - outBounds[0] + 1;
    int offset = outBounds[0] - inBounds[0];
    b->SetNumberOfTuples(tuples);
    memcpy(b->GetVoidPointer(0), a->GetVoidPointer(offset), tuples*tupleSize);
    return b;
    }  
}

//----------------------------------------------------------------------------
int vtkXMLRectilinearGridWriter::WriteAppendedMode(vtkIndent indent)
{
  int i;
  this->CoordinatePositions = new unsigned long*[this->NumberOfPieces];
  for(i=0;i < this->NumberOfPieces;++i) { this->CoordinatePositions[i] = 0; }
  
  int result = this->Superclass::WriteAppendedMode(indent);
  
  for(i=0;i < this->NumberOfPieces;++i)
    {
    if(this->CoordinatePositions[i])
      {
      delete [] this->CoordinatePositions[i];
      }
    }
  delete [] this->CoordinatePositions;
  return result;
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridWriter::WriteAppendedPiece(int index,
                                                     vtkIndent indent)
{
  this->Superclass::WriteAppendedPiece(index, indent);  
  this->CoordinatePositions[index] =
    this->WriteCoordinatesAppended(this->GetInput()->GetXCoordinates(),
                                   this->GetInput()->GetYCoordinates(),
                                   this->GetInput()->GetZCoordinates(),
                                   indent);
  
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridWriter::WriteAppendedPieceData(int index)
{
  // Split progress range by the approximate fractions of data written
  // by each step in this method.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[3];
  this->CalculateSuperclassFraction(fractions);
  
  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Let the superclass write its data.
  this->Superclass::WriteAppendedPieceData(index);
  
  // Set the range of progress for the coordinates arrays.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the coordinates arrays.
  this->WriteCoordinatesAppendedData(this->GetInput()->GetXCoordinates(),
                                     this->GetInput()->GetYCoordinates(),
                                     this->GetInput()->GetZCoordinates(),
                                     this->CoordinatePositions[index]);
  this->CoordinatePositions[index] = 0;
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridWriter::WriteInlinePiece(int index, vtkIndent indent)
{
  // Split progress range by the approximate fractions of data written
  // by each step in this method.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[3];
  this->CalculateSuperclassFraction(fractions);
  
  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Let the superclass write its data.
  this->Superclass::WriteInlinePiece(index, indent);
  
  // Set the range of progress for the coordinates arrays.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the coordinates arrays.
  this->WriteCoordinatesInline(this->GetInput()->GetXCoordinates(),
                               this->GetInput()->GetYCoordinates(),
                               this->GetInput()->GetZCoordinates(),
                               indent);
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridWriter::CalculateSuperclassFraction(float* fractions)
{
  int extent[6];
  this->ExtentTranslator->GetExtent(extent);
  int dims[3] = {extent[1]-extent[0]+1,
                 extent[3]-extent[2]+1,
                 extent[5]-extent[4]+1};
  
  // The amount of data written by the superclass comes from the
  // point/cell data arrays.
  vtkIdType superclassPieceSize = 
    (this->GetInput()->GetPointData()->GetNumberOfArrays()*dims[0]*dims[1]*dims[2]+
     this->GetInput()->GetCellData()->GetNumberOfArrays()*(dims[0]-1)*(dims[1]-1)*(dims[2]-1));
  
  // The total data written includes the coordinate arrays.
  vtkIdType totalPieceSize =
    superclassPieceSize + dims[0] + dims[1] + dims[2];
  if(totalPieceSize == 0)
    {
    totalPieceSize = 1;
    }
  fractions[0] = 0;
  fractions[1] = fractions[0] + float(superclassPieceSize)/totalPieceSize;
  fractions[2] = 1;
}
