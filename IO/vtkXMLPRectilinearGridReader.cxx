/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPRectilinearGridReader.cxx
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
#include "vtkXMLPRectilinearGridReader.h"
#include "vtkObjectFactory.h"
#include "vtkXMLRectilinearGridReader.h"
#include "vtkRectilinearGrid.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkXMLPRectilinearGridReader, "1.1");
vtkStandardNewMacro(vtkXMLPRectilinearGridReader);

//----------------------------------------------------------------------------
vtkXMLPRectilinearGridReader::vtkXMLPRectilinearGridReader()
{
  // Copied from vtkRectilinearGridReader constructor:
  this->SetOutput(vtkRectilinearGrid::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkXMLPRectilinearGridReader::~vtkXMLPRectilinearGridReader()
{
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridReader::SetOutput(vtkRectilinearGrid *output)
{
  this->Superclass::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkXMLPRectilinearGridReader::GetOutput()
{
  if(this->NumberOfOutputs < 1)
    {
    return 0;
    }
  return static_cast<vtkRectilinearGrid*>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkXMLPRectilinearGridReader::GetPieceInput(int index)
{
  vtkXMLRectilinearGridReader* reader =
    static_cast<vtkXMLRectilinearGridReader*>(this->PieceReaders[index]);
  return reader->GetOutput();
}

//----------------------------------------------------------------------------
const char* vtkXMLPRectilinearGridReader::GetDataSetName()
{
  return "PRectilinearGrid";
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridReader::SetOutputExtent(int* extent)
{
  this->GetOutput()->SetExtent(extent);
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridReader::GetPieceInputExtent(int index, int* extent)
{
  this->GetPieceInput(index)->GetExtent(extent);
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridReader::SetupOutputInformation()
{
  this->Superclass::SetupOutputInformation();  
  vtkRectilinearGrid* output = this->GetOutput();
  
  // Create the coordinate arrays.
  vtkDataArray* x = vtkFloatArray::New();
  vtkDataArray* y = vtkFloatArray::New();
  vtkDataArray* z = vtkFloatArray::New();
  output->SetXCoordinates(x);
  output->SetYCoordinates(y);
  output->SetZCoordinates(z);
  x->Delete();
  y->Delete();
  z->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  
  // Allocate the coordinate arrays.
  vtkRectilinearGrid* output = this->GetOutput();  
  output->GetXCoordinates()->SetNumberOfTuples(this->PointDimensions[0]);
  output->GetYCoordinates()->SetNumberOfTuples(this->PointDimensions[1]);
  output->GetZCoordinates()->SetNumberOfTuples(this->PointDimensions[2]);  
}

//----------------------------------------------------------------------------
int vtkXMLPRectilinearGridReader::ReadPieceData()
{
  if(!this->Superclass::ReadPieceData()) { return 0; }
  
  // Copy the coordinates arrays from the input piece.
  vtkRectilinearGrid* input = this->GetPieceInput(this->Piece);
  vtkRectilinearGrid* output = this->GetOutput();
  this->CopySubCoordinates(this->SubPieceExtent, this->UpdateExtent,
                           this->SubExtent, input->GetXCoordinates(),
                           output->GetXCoordinates());
  this->CopySubCoordinates(this->SubPieceExtent+2, this->UpdateExtent+2,
                           this->SubExtent+2, input->GetYCoordinates(),
                           output->GetYCoordinates());
  this->CopySubCoordinates(this->SubPieceExtent+4, this->UpdateExtent+4,
                           this->SubExtent+4, input->GetZCoordinates(),
                           output->GetZCoordinates());
  
  return 1;
}

//----------------------------------------------------------------------------
vtkXMLDataReader* vtkXMLPRectilinearGridReader::CreatePieceReader()
{
  return vtkXMLRectilinearGridReader::New();
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridReader::CopySubCoordinates(int* inBounds,
                                                      int* outBounds,
                                                      int* subBounds,
                                                      vtkDataArray* inArray,
                                                      vtkDataArray* outArray)
{
  unsigned int components = inArray->GetNumberOfComponents();
  unsigned int tupleSize = inArray->GetDataTypeSize()*components;
  
  int destStartIndex = subBounds[0] - outBounds[0];
  int sourceStartIndex = subBounds[0] - inBounds[0];
  int length = subBounds[1] - subBounds[0] + 1;
  
  memcpy(outArray->GetVoidPointer(destStartIndex*components),
         inArray->GetVoidPointer(sourceStartIndex*components),
         length*tupleSize);
}
