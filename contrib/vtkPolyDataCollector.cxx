/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataCollector.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPolyDataCollector.h"
#include "vtkExtent.h"
#include "vtkUnstructuredInformation.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPolyDataCollector* vtkPolyDataCollector::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPolyDataCollector");
  if(ret)
    {
    return (vtkPolyDataCollector*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPolyDataCollector;
}




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



