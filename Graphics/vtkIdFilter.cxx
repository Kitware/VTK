/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdFilter.cxx
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
#include "vtkIdFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkIdFilter* vtkIdFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkIdFilter");
  if(ret)
    {
    return (vtkIdFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkIdFilter;
}




// Construct object with PointIds and CellIds on; and ids being generated
// as scalars.
vtkIdFilter::vtkIdFilter()
{
  this->PointIds = 1;
  this->CellIds = 1;
  this->FieldData = 0;
  this->SetIdsArrayName("vtkIdFilter_Ids");
}

vtkIdFilter::~vtkIdFilter()
{
  if (this->IdsArrayName)
    {
    delete []IdsArrayName;
    }
}

// 
// Map ids into attribute data
//
void vtkIdFilter::Execute()
{
  vtkIdType numPts, numCells, id;
  vtkIdTypeArray *ptIds;
  vtkIdTypeArray *cellIds;
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
  vtkPointData *inPD=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *inCD=input->GetCellData(), *outCD=output->GetCellData();

  // Initialize
  //
  vtkDebugMacro(<<"Generating ids!");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  // Loop over points (if requested) and generate ids
  //
  if ( this->PointIds && numPts > 0 )
    {
    ptIds = vtkIdTypeArray::New();
    ptIds->SetNumberOfValues(numPts);

    for (id=0; id < numPts; id++)
      {
      ptIds->SetValue(id, id);
      }

    if ( ! this->FieldData )
      {
      outPD->SetScalars(ptIds);
      outPD->CopyScalarsOff();
      }
    else
      {
      ptIds->SetName(this->IdsArrayName);
      outPD->AddArray(ptIds);
      outPD->CopyFieldOff(this->IdsArrayName);
      }
    ptIds->Delete();
    }

  // Loop over cells (if requested) and generate ids
  //
  if ( this->CellIds && numCells > 0 )
    {
    cellIds = vtkIdTypeArray::New();
    cellIds->SetNumberOfValues(numCells);

    for (id=0; id < numCells; id++)
      {
      cellIds->SetValue(id, id);
      }

    if ( ! this->FieldData )
      {
      outCD->SetScalars(cellIds);
      outCD->CopyScalarsOff();
      }
    else
      {
      cellIds->SetName(this->IdsArrayName);
      outCD->AddArray(cellIds);
      outCD->CopyFieldOff(this->IdsArrayName);
      }
    cellIds->Delete();
    }

  outPD->PassData(inPD);
  outCD->PassData(inCD);
}

void vtkIdFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Point Ids: "    << (this->PointIds ? "On\n" : "Off\n");
  os << indent << "Cell Ids: "     << (this->CellIds ? "On\n" : "Off\n");
  os << indent << "Field Data: "   << (this->FieldData ? "On\n" : "Off\n");
  os << indent << "IdsArrayName: " << (this->IdsArrayName ? this->IdsArrayName
       : "(none)") << "\n";
}
