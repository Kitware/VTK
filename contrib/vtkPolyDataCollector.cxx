/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataCollector.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkPolyDataCollector.h"
#include "vtkExtent.h"
#include "vtkUnstructuredInformation.h"

//----------------------------------------------------------------------------
vtkPolyDataCollector::vtkPolyDataCollector()
{
}

//----------------------------------------------------------------------------
vtkPolyDataCollector::~vtkPolyDataCollector()
{
}

//----------------------------------------------------------------------------
void vtkPolyDataCollector::SetInputMemoryLimit(unsigned long limit)
{
  vtkPolyData *input = this->GetInput();

  if (input == NULL)
    {
    vtkErrorMacro("No Input");
    return ;
    }
  
  input->SetMemoryLimit(limit);
}


//----------------------------------------------------------------------------
int vtkPolyDataCollector::GetNumberOfStreamDivisions()
{
  int num, max;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();

  if (input == NULL)
    {
    return 1;
    }
  
  max = input->GetUnstructuredInformation()->GetMaximumNumberOfPieces();
  num = output->GetEstimatedUpdateMemorySize();
  num = 1 + num / input->GetMemoryLimit();
  
  if (num > max)
    {
    num = max;
    }
  
  return num;
}

//----------------------------------------------------------------------------
int vtkPolyDataCollector::ComputeDivisionExtents(vtkDataObject *dataOut, 
						 int division, int numDivisions)
{
  vtkPolyData *output = (vtkPolyData *)(dataOut);
  vtkPolyData *input = this->GetInput();
  int piece, numPieces;
  
  if (input == NULL)
    {
    vtkErrorMacro("No Input");
    return 0;
    }
  
  output->GetUpdateExtent(piece, numPieces);
  numPieces *= numDivisions;
  piece = piece * numDivisions + division;

  input->SetUpdateExtent(piece, numPieces);

  if (piece < input->GetUnstructuredInformation()->GetMaximumNumberOfPieces())
    {
    return 1;
    }
  
  return 0;
}


//----------------------------------------------------------------------------
void vtkPolyDataCollector::StreamExecuteStart()
{
  this->AppendFilter = vtkAppendPolyData::New();
}

//----------------------------------------------------------------------------
void vtkPolyDataCollector::StreamExecuteEnd()
{
  vtkPolyData *results = this->AppendFilter->GetOutput();
  vtkPolyData *output = this->GetOutput();
  
  results->PreUpdate();
  results->InternalUpdate();
  
  output->CopyStructure(results);
  output->GetPointData()->PassData(results->GetPointData());
  output->GetCellData()->PassData(results->GetCellData());
  
  this->AppendFilter->Delete();
  this->AppendFilter = NULL; 
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
void vtkPolyDataCollector::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *copy;
  
  copy = vtkPolyData::New();
  
  copy->CopyStructure(input);
  copy->GetPointData()->PassData(input->GetPointData());
  copy->GetCellData()->PassData(input->GetCellData());
  
  this->AppendFilter->AddInput(copy);
  copy->Delete();
}


//----------------------------------------------------------------------------
void vtkPolyDataCollector::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);
}



