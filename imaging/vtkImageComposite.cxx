/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageComposite.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
  int numPts = output->GetNumberOfPoints();
  vtkImageData *input;
  int i, j;
  int firstFlag = 1;
  vtkScalars *inPScalars;
  vtkDataArray *inZData;
  vtkFloatArray *outZArray;
  vtkFieldData *outZField;
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
  outZField = vtkFieldData::New();
  outZField->SetArray(0, outZArray);
  outZField->SetArrayName(0, "ZBuffer");

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
  output->GetPointData()->SetFieldData(outZField);
  outPScalars->Delete();
  outZField->Delete();
  outZArray->Delete();

}


//----------------------------------------------------------------------------
void vtkImageComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSource::PrintSelf(os,indent);
}

