/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageComposite.cxx
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
#include "vtkImageComposite.h"
#include "vtkStructuredPoints.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"

//----------------------------------------------------------------------------
vtkImageComposite::vtkImageComposite()
{
  this->NumberOfRequiredInputs = 1;
  this->SetOutput(vtkStructuredPoints::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkImageComposite::~vtkImageComposite()
{
}

//----------------------------------------------------------------------------
void vtkImageComposite::SetOutput(vtkStructuredPoints *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkStructuredPoints *vtkImageComposite::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkStructuredPoints *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
void vtkImageComposite::AddInput(vtkImageData *ds)
{
  this->vtkProcessObject::AddInput(ds);
}

//----------------------------------------------------------------------------
void vtkImageComposite::RemoveInput(vtkImageData *ds)
{
  this->vtkProcessObject::RemoveInput(ds);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageComposite::GetInput(int idx)
{
  if (idx >= this->NumberOfInputs || idx < 0)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[idx]);
}

//----------------------------------------------------------------------------
void vtkImageComposite::Execute()
{
  vtkStructuredPoints *output = this->GetOutput();
  vtkIdType numPts = output->GetNumberOfPoints();
  vtkImageData *input;
  int i;
  vtkIdType j;
  int firstFlag = 1;
  vtkScalars *inPScalars;
  vtkDataArray *inZData;
  vtkFloatArray *outZArray;
  float *outZPtr, *inZ, *outZ;
  vtkScalars *outPScalars;
  int alphaFlag = 0;
  float alpha, oneMinusAlpha;

  // Since this is not an image filter, we need to allocate.
  input = this->GetInput(0);
  numPts = input->GetNumberOfPoints();
  output->SetDimensions(input->GetDimensions());
  output->SetSpacing(input->GetSpacing());
  output->SetNumberOfScalarComponents(input->GetNumberOfScalarComponents());
  if (input->GetNumberOfScalarComponents() == 4)
    {
    alphaFlag = 1;
    }
  
  // allocate the output
  outZArray = vtkFloatArray::New();
  outZArray->Allocate(numPts);
  outZArray->SetNumberOfTuples(numPts);
  outZPtr = outZArray->WritePointer(0, numPts);
  outZArray->SetName("ZBuffer");
  output->GetPointData()->AddArray(outZArray);

  outPScalars = vtkScalars::New();
  if (alphaFlag)
    {
    outPScalars->SetDataType(VTK_FLOAT);
    output->SetScalarType(VTK_FLOAT);
    }
  else
    {
    outPScalars->SetDataType(VTK_UNSIGNED_CHAR);
    output->SetScalarType(VTK_UNSIGNED_CHAR);
    }
  
  outPScalars->SetNumberOfComponents(3+alphaFlag);  
  outPScalars->SetNumberOfScalars(numPts);

  // composite each input
  for (i = 0; i < this->NumberOfInputs; ++i)
    {
    input = (vtkImageData*)(this->Inputs[i]);
    if (input && input->GetPointData()->GetScalars() && 
        input->GetPointData()->GetFieldData()) 
      {
      if (input->GetNumberOfPoints() != numPts)
        {
        vtkErrorMacro("PointMismatch.");
        continue;
        }
      inPScalars = input->GetPointData()->GetScalars();
      if (!alphaFlag && (inPScalars->GetDataType() != VTK_UNSIGNED_CHAR ||
          inPScalars->GetNumberOfComponents() != 3))
        {
        vtkErrorMacro("Bap Pixel data format.");
        continue;
        }
      if (alphaFlag && (inPScalars->GetDataType() != VTK_FLOAT ||
          inPScalars->GetNumberOfComponents() != 4))
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
      if (alphaFlag)
	{
	float *outPPtr, *inP, *outP;
	outPPtr = (float*)(outPScalars->GetVoidPointer(0));
	outP = outPPtr;
	inP = (float*)(inPScalars->GetVoidPointer(0));
	for (j = 0; j < numPts; ++j)
	  {
	  if (firstFlag)
	    { // copy
	    *outZ++ = *inZ++;
	    *outP++ = *inP++;
	    *outP++ = *inP++;
	    *outP++ = *inP++;
	    *outP++ = *inP++;
	    }
	  else if (*inZ <= *outZ)
	    { // composite
	    alpha = inZ[3];
	    oneMinusAlpha = 1.0 - alpha;
	    *outP = *outP * oneMinusAlpha + *inP * alpha;
	    ++outP;  ++inP;
	    *outP = *outP * oneMinusAlpha + *inP * alpha;
	    ++outP;  ++inP;
	    *outP = *outP * oneMinusAlpha + *inP * alpha;
	    ++outP;  ++inP;
	    *outP = *outP * oneMinusAlpha + *inP * alpha;
	    ++outP;  ++inP;
	    *outZ++ = *inZ++;
	    }
	  else
	    { // skip
	    ++outZ;
	    ++inZ;
	    outP += 4;
	    inP += 4;
	    }
          }
        }
      else
	{
	unsigned char *outPPtr, *inP, *outP;
	outPPtr = (unsigned char *)outPScalars->GetVoidPointer(0);
	outP = outPPtr;
	inP = (unsigned char *)(inPScalars->GetVoidPointer(0));
	for (j = 0; j < numPts; ++j)
	  {
	  if (firstFlag || *inZ <= *outZ)
	    { // copy
	    *outZ++ = *inZ++;
	    *outP++ = *inP++;
	    *outP++ = *inP++;
	    *outP++ = *inP++;
	    }
	  else
	    { // skip
	    ++outZ;
	    ++inZ;
	    outP += 3;
	    inP += 3;
	    }
          }
        }      
      
      firstFlag = 0;
      }
    }
  output->GetPointData()->SetScalars(outPScalars);
  outPScalars->Delete();
  outZArray->Delete();
}

//----------------------------------------------------------------------------
void vtkImageComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSource::PrintSelf(os,indent);
}
