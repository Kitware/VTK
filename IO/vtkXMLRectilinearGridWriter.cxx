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
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkExtentTranslator.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkXMLRectilinearGridWriter, "1.2");
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
void vtkXMLRectilinearGridWriter::WriteAppendedMode(vtkIndent indent)
{
  int i;
  this->CoordinatePositions = new unsigned long*[this->NumberOfPieces];
  for(i=0;i < this->NumberOfPieces;++i) { this->CoordinatePositions[i] = 0; }
  
  this->Superclass::WriteAppendedMode(indent);
  
  for(i=0;i < this->NumberOfPieces;++i)
    {
    if(this->CoordinatePositions[i])
      {
      delete [] this->CoordinatePositions[i];
      }
    }
  delete [] this->CoordinatePositions;
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
  this->Superclass::WriteAppendedPieceData(index);
  this->WriteCoordinatesAppendedData(this->GetInput()->GetXCoordinates(),
                                     this->GetInput()->GetYCoordinates(),
                                     this->GetInput()->GetZCoordinates(),
                                     this->CoordinatePositions[index]);
  this->CoordinatePositions[index] = 0;
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridWriter::WriteInlinePiece(int index, vtkIndent indent)
{
  this->Superclass::WriteInlinePiece(index, indent);
  this->WriteCoordinatesInline(this->GetInput()->GetXCoordinates(),
                               this->GetInput()->GetYCoordinates(),
                               this->GetInput()->GetZCoordinates(),
                               indent);
}
