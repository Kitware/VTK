/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLStructuredGridWriter.cxx
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
#include "vtkXMLStructuredGridWriter.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredGrid.h"

vtkCxxRevisionMacro(vtkXMLStructuredGridWriter, "1.1");
vtkStandardNewMacro(vtkXMLStructuredGridWriter);

//----------------------------------------------------------------------------
vtkXMLStructuredGridWriter::vtkXMLStructuredGridWriter()
{
  this->PointsPosition = 0;
}

//----------------------------------------------------------------------------
vtkXMLStructuredGridWriter::~vtkXMLStructuredGridWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::SetInput(vtkStructuredGrid* input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLStructuredGridWriter::GetInput()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkStructuredGrid*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::GetInputExtent(int* extent)
{
  this->GetInput()->GetExtent(extent);
}

//----------------------------------------------------------------------------
const char* vtkXMLStructuredGridWriter::GetDataSetName()
{
  return "StructuredGrid";
}

//----------------------------------------------------------------------------
const char* vtkXMLStructuredGridWriter::GetDefaultFileExtension()
{
  return "vts";
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::WriteAppendedMode(vtkIndent indent)
{
  this->PointsPosition = new unsigned long[this->NumberOfPieces];
  this->Superclass::WriteAppendedMode(indent);
  delete [] this->PointsPosition;
  this->PointsPosition = 0;
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::WriteAppendedPiece(int index,
                                                    vtkIndent indent)
{
  this->Superclass::WriteAppendedPiece(index, indent);
  this->PointsPosition[index] =
    this->WritePointsAppended(this->GetInput()->GetPoints(), indent);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::WriteAppendedPieceData(int index)
{
  this->Superclass::WriteAppendedPieceData(index);
  this->WritePointsAppendedData(this->GetInput()->GetPoints(),
                                this->PointsPosition[index]);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::WriteInlinePiece(int index, vtkIndent indent)
{
  this->Superclass::WriteInlinePiece(index, indent);
  this->WritePointsInline(this->GetInput()->GetPoints(), indent);
}
