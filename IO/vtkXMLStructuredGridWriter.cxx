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

#include "vtkCellData.h"
#include "vtkErrorCode.h"
#include "vtkExtentTranslator.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStructuredGrid.h"

vtkCxxRevisionMacro(vtkXMLStructuredGridWriter, "1.4");
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
int vtkXMLStructuredGridWriter::WriteAppendedMode(vtkIndent indent)
{
  this->PointsPosition = new unsigned long[this->NumberOfPieces];
  int result = this->Superclass::WriteAppendedMode(indent);
  delete [] this->PointsPosition;
  this->PointsPosition = 0;
  return result;
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::WriteAppendedPiece(int index,
                                                    vtkIndent indent)
{
  this->Superclass::WriteAppendedPiece(index, indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  this->PointsPosition[index] =
    this->WritePointsAppended(this->GetInput()->GetPoints(), indent);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::WriteAppendedPieceData(int index)
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
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  
  // Set the range of progress for the points array.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the points array.
  this->WritePointsAppendedData(this->GetInput()->GetPoints(),
                                this->PointsPosition[index]);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::WriteInlinePiece(int index, vtkIndent indent)
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
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  
  // Set the range of progress for the points array.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the points array.
  this->WritePointsInline(this->GetInput()->GetPoints(), indent);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridWriter::CalculateSuperclassFraction(float* fractions)
{
  int extent[6];
  this->ExtentTranslator->GetExtent(extent);
  int dims[3] = {extent[1]-extent[0],
                 extent[3]-extent[2],
                 extent[5]-extent[4]};
  
  // The amount of data written by the superclass comes from the
  // point/cell data arrays.
  vtkIdType superclassPieceSize = 
    (this->GetInput()->GetPointData()->GetNumberOfArrays()*dims[0]*dims[1]*dims[2]+
     this->GetInput()->GetCellData()->GetNumberOfArrays()*(dims[0]-1)*(dims[1]-1)*(dims[2]-1));
  
  // The total data written includes the points array.
  vtkIdType totalPieceSize =
    superclassPieceSize + (dims[0] * dims[1] * dims[2]);
  if(totalPieceSize == 0)
    {
    totalPieceSize = 1;
    }
  fractions[0] = 0;
  fractions[1] = fractions[0] + float(superclassPieceSize)/totalPieceSize;
  fractions[2] = 1;
}
