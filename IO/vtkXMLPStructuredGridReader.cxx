/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredGridReader.cxx
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
#include "vtkXMLPStructuredGridReader.h"
#include "vtkObjectFactory.h"
#include "vtkXMLStructuredGridReader.h"
#include "vtkStructuredGrid.h"
#include "vtkPoints.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkXMLPStructuredGridReader, "1.1");
vtkStandardNewMacro(vtkXMLPStructuredGridReader);

//----------------------------------------------------------------------------
vtkXMLPStructuredGridReader::vtkXMLPStructuredGridReader()
{
  // Copied from vtkStructuredGridReader constructor:
  this->SetOutput(vtkStructuredGrid::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkXMLPStructuredGridReader::~vtkXMLPStructuredGridReader()
{
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::SetOutput(vtkStructuredGrid *output)
{
  this->Superclass::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLPStructuredGridReader::GetOutput()
{
  if(this->NumberOfOutputs < 1)
    {
    return 0;
    }
  return static_cast<vtkStructuredGrid*>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLPStructuredGridReader::GetPieceInput(int index)
{
  vtkXMLStructuredGridReader* reader =
    static_cast<vtkXMLStructuredGridReader*>(this->PieceReaders[index]);
  return reader->GetOutput();
}

//----------------------------------------------------------------------------
const char* vtkXMLPStructuredGridReader::GetDataSetName()
{
  return "PStructuredGrid";
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::SetOutputExtent(int* extent)
{
  this->GetOutput()->SetExtent(extent);
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::GetPieceInputExtent(int index, int* extent)
{
  this->GetPieceInput(index)->GetExtent(extent);
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::SetupOutputInformation()
{
  this->Superclass::SetupOutputInformation();  
  vtkStructuredGrid* output = this->GetOutput();
  
  // Create the points array.
  vtkPoints* points = vtkPoints::New();
  
  vtkDataArray* a = vtkFloatArray::New();
  a->SetNumberOfComponents(3);
  points->SetData(a);
  a->Delete();
  
  output->SetPoints(points);
  points->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  
  // Allocate the points array.
  vtkDataArray* a = this->GetOutput()->GetPoints()->GetData();
  a->SetNumberOfTuples(this->GetNumberOfPoints());  
}

//----------------------------------------------------------------------------
int vtkXMLPStructuredGridReader::ReadPieceData()
{
  if(!this->Superclass::ReadPieceData()) { return 0; }
  
  // Copy the points.
  vtkStructuredGrid* input = this->GetPieceInput(this->Piece);
  vtkStructuredGrid* output = this->GetOutput();  
  this->CopyArrayForPoints(input->GetPoints()->GetData(),
                           output->GetPoints()->GetData());
  
  return 1;
}

//----------------------------------------------------------------------------
vtkXMLDataReader* vtkXMLPStructuredGridReader::CreatePieceReader()
{
  return vtkXMLStructuredGridReader::New();
}
