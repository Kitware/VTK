/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlankStructuredGrid.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkBlankStructuredGrid.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------
vtkBlankStructuredGrid* vtkBlankStructuredGrid::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkBlankStructuredGrid");
  if(ret)
    {
    return (vtkBlankStructuredGrid*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkBlankStructuredGrid;
}

// Construct object to extract all of the input data.
vtkBlankStructuredGrid::vtkBlankStructuredGrid()
{
  this->MinBlankingValue = VTK_LARGE_FLOAT;
  this->MaxBlankingValue = VTK_LARGE_FLOAT;
  this->ArrayName = NULL;
  this->ArrayId = -1;
  this->Component = 0;
}

vtkBlankStructuredGrid::~vtkBlankStructuredGrid()
{
  if ( this->ArrayName )
    {
    delete [] this->ArrayName;
    this->ArrayName = NULL;
    }
}


template <class T>
static void vtkBlankStructuredGridExecute(vtkBlankStructuredGrid *vtkNotUsed(self),
                                          T *dptr, int numPts, int numComp,
                                          int comp, float min, float max,
                                          vtkUnsignedCharArray *blanking)
{
  T compValue;
  dptr += comp;

  for ( int ptId=0; ptId < numPts; ptId++, dptr+=numComp)
    {
    compValue = *dptr;
    if ( compValue >= min && compValue <= max )
      {
      blanking->SetValue(ptId,0); //make it invisible
      }
    else
      {
      blanking->SetValue(ptId,1);
      }
    }
}


void vtkBlankStructuredGrid::Execute()
{
  vtkStructuredGrid *input= this->GetInput();
  vtkPointData *pd=input->GetPointData();
  vtkCellData *cd=input->GetCellData();
  vtkStructuredGrid *output= this->GetOutput();
  vtkPointData *outPD=output->GetPointData();
  vtkCellData *outCD=output->GetCellData();
  int numPts = input->GetNumberOfPoints();
  vtkDataArray *dataArray=NULL;
  int numComp;

  vtkDebugMacro(<< "Blanking Grid");

  // Pass input to output
  //
  output->CopyStructure(input);
  outPD->PassData(pd);
  outCD->PassData(cd);

  // Get the appropriate data array
  //
  if ( this->ArrayName != NULL )
    {
    dataArray = pd->GetArray(this->ArrayName);
    }
  else if ( this->ArrayId >= 0 )
    {
    dataArray = pd->GetArray(this->ArrayId);
    }
    
  if ( !dataArray || 
       (numComp=dataArray->GetNumberOfComponents()) <= this->Component )
    {
    vtkWarningMacro(<<"Data array not found");
    return;
    }
  void *dptr = dataArray->GetVoidPointer(0);
  
  // Loop over the data array setting anything within the data range specified
  // to be blanked.
  //
  vtkUnsignedCharArray *blanking = vtkUnsignedCharArray::New();
  blanking->SetNumberOfValues(numPts);

  // call templated function
  switch (dataArray->GetDataType())
    {
    vtkTemplateMacro8(vtkBlankStructuredGridExecute, this, (VTK_TT *)(dptr), numPts,
                      numComp, this->Component, this->MinBlankingValue, 
                      this->MaxBlankingValue, blanking);
    default:
      break;
    }

  // Clean up and get out
  output->SetPointVisibility(blanking);
  blanking->Delete();
  output->BlankingOn();
}


void vtkBlankStructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredGridToStructuredGridFilter::PrintSelf(os,indent);

  os << indent << "Min Blanking Value: " << this->MinBlankingValue << "\n";
  os << indent << "Max Blanking Value: " << this->MaxBlankingValue << "\n";
  os << indent << "Array Name: ";
  if ( this->ArrayName )
    {
    os << this->ArrayName << "\n";
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "Array ID: " << this->ArrayId << "\n";
  os << indent << "Component: " << this->Component << "\n";
  
}


