/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeFilter.cxx
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
#include "vtkCompositeFilter.h"
#include "vtkStructuredPoints.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkCompositeFilter* vtkCompositeFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCompositeFilter");
  if(ret)
    {
    return (vtkCompositeFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCompositeFilter;
}




//----------------------------------------------------------------------------
vtkCompositeFilter::vtkCompositeFilter()
{
}

//----------------------------------------------------------------------------
vtkCompositeFilter::~vtkCompositeFilter()
{
}

//----------------------------------------------------------------------------
void vtkCompositeFilter::AddInput(vtkStructuredPoints *ds)
{
  this->vtkProcessObject::AddInput(ds);
}

//----------------------------------------------------------------------------
void vtkCompositeFilter::RemoveInput(vtkStructuredPoints *ds)
{
  this->vtkProcessObject::RemoveInput(ds);
}

//----------------------------------------------------------------------------
vtkStructuredPoints *vtkCompositeFilter::GetInput(int idx)
{
  if (idx >= this->NumberOfInputs || idx < 0)
    {
    return NULL;
    }
  
  return (vtkStructuredPoints *)(this->Inputs[idx]);
}

//----------------------------------------------------------------------------
void vtkCompositeFilter::Execute()
{
  vtkStructuredPoints *output = this->GetOutput();
  vtkIdType numPts = output->GetNumberOfPoints, j;
  vtkStructuredPoints *input;
  int i;
  int firstFlag = 1;
  vtkScalars *inPScalars;
  vtkDataArray *inZData;
  vtkFloatArray *outZArray;
  vtkFieldData *outZField;
  float *outZPtr, *inZ, *outZ;
  vtkScalars *outPScalars;
  unsigned char *outPPtr, *inP, *outP;

  // Since this is not an image filter, we need to allocate.
  input = this->GetInput(0);
  numPts = input->GetNumberOfPoints();
  output->SetDimensions(input->GetDimensions());
  output->SetSpacing(input->GetSpacing());

  // allocate the output
  outZArray = vtkFloatArray::New();
  outZArray->Allocate(numPts);
  outZArray->SetNumberOfTuples(numPts);
  outZPtr = outZArray->WritePointer(0, numPts);
  outZField = vtkFieldData::New();
  outZField->SetArray(0, outZArray);
  outZField->SetArrayName(0, "ZBuffer");

  outPScalars = vtkScalars::New();
  outPScalars->SetDataType(VTK_UNSIGNED_CHAR);
  outPScalars->SetNumberOfComponents(3);  
  outPScalars->SetNumberOfScalars(numPts);
  outPPtr = (unsigned char *)(outPScalars->GetVoidPointer(0));

  // composite each input
  for (i = 0; i < this->NumberOfInputs; ++i)
    {
    input = (vtkStructuredPoints*)(this->Inputs[i]);
    if (input && input->GetPointData()->GetScalars() && 
        input->GetPointData()->GetFieldData()) 
      {
      if (input->GetNumberOfPoints() != numPts)
        {
        vtkErrorMacro("PointMismatch.");
        continue;
        }
      inPScalars = input->GetPointData()->GetScalars();
      if (inPScalars->GetDataType() != VTK_UNSIGNED_CHAR ||
          inPScalars->GetNumberOfComponents() != 3)
        {
        vtkErrorMacro("Bap Pixel data format.");
        continue;
        }
      inZData = input->GetPointData()->GetFieldData()->GetArray("ZBuffer");
      if (inZData == NULL || inZData->GetDataType() != VTK_FLOAT)
        {
        vtkErrorMacro("Bad z data format");
        continue;
        }

      outZ = outZPtr;
      inZ = ((vtkFloatArray*)inZData)->GetPointer(0);
      outP = outPPtr;
      inP = (unsigned char *)(inPScalars->GetVoidPointer(0));
      
      for (j = 0; j < numPts; ++j)
        {
        if (firstFlag || *inZ < *outZ)
          {
          *outZ++ = *inZ++;
          *outP++ = *inP++;
          *outP++ = *inP++;
          *outP++ = *inP++;
          }
        else
          {
          ++outZ;
          ++inZ;
          outP += 3;
          inP += 3;
          }
        }
        firstFlag = 0;
      }
    }
  output->GetPointData()->SetScalars(outPScalars);
  output->GetPointData()->SetFieldData(outZField);
  outPScalars->Delete();
  outZField->Delete();
  outZArray->Delete();

}



//----------------------------------------------------------------------------
int vtkCompositeFilter::ComputeInputUpdateExtents(vtkDataObject *data)
{
  vtkStructuredPoints *output = (vtkStructuredPoints*)data;
  vtkStructuredPoints *input;
  int i;

  for ( i = 0; i < this->NumberOfInputs; ++i)
    {
    input = this->GetInput(i);
    if (input)
      {  
      input->CopyUpdateExtent(output);
      }
    }
  return 1;
}


//----------------------------------------------------------------------------
void vtkCompositeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsToStructuredPointsFilter::PrintSelf(os,indent);
}

