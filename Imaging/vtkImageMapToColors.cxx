/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToColors.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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
#include "vtkImageMapToColors.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkImageMapToColors* vtkImageMapToColors::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageMapToColors");
  if(ret)
    {
    return (vtkImageMapToColors*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageMapToColors;
}

//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageMapToColors::vtkImageMapToColors()
{
  this->OutputFormat = 4;
  this->ActiveComponent = 0;
  this->PassAlphaToOutput = 0;
  this->LookupTable = NULL;
  this->DataWasPassed = 0;
}


vtkImageMapToColors::~vtkImageMapToColors()
{
  if (this->LookupTable != NULL) 
    {
    this->LookupTable->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
unsigned long vtkImageMapToColors::GetMTime()
{
  unsigned long t1, t2;

  t1 = this->vtkImageToImageFilter::GetMTime();
  if (this->LookupTable)
    {
    t2 = this->LookupTable->GetMTime();
    if (t2 > t1)
      {
      t1 = t2;
      }
    }
  return t1;
}

//----------------------------------------------------------------------------
// This method checks to see if we can simply reference the input data
void vtkImageMapToColors::ExecuteData(vtkDataObject *output)
{
  vtkImageData *outData = (vtkImageData *)(output);
  vtkImageData *inData = this->GetInput();
 
  // If LookupTable is null, just pass the data
  if (this->LookupTable == NULL)
    {
    vtkDebugMacro("ExecuteData: LookupTable not set, "\
		  "passing input to output.");

    outData->SetExtent(inData->GetExtent());
    outData->GetPointData()->PassData(inData->GetPointData());
    this->DataWasPassed = 1;
    }
  else // normal behaviour
    {
    if (this->DataWasPassed)
      {
      outData->GetPointData()->SetScalars(NULL);
      this->DataWasPassed = 0;
      }
    
    this->vtkImageToImageFilter::ExecuteData(output);
    }
}

//----------------------------------------------------------------------------
void vtkImageMapToColors::ExecuteInformation(vtkImageData *inData, 
					     vtkImageData *outData)
{
  int numComponents = 4;

  switch (this->OutputFormat)
    {
    case VTK_RGBA:
      numComponents = 4;
      break;
    case VTK_RGB:
      numComponents = 3;
      break;
    case VTK_LUMINANCE_ALPHA:
      numComponents = 2;
      break;
    case VTK_LUMINANCE:
      numComponents = 1;
      break;
    default:
      vtkErrorMacro("ExecuteInformation: Unrecognized color format.");
      break;
    }

  if (this->LookupTable == NULL)
    {
    if (inData->GetScalarType() != VTK_UNSIGNED_CHAR)
      {
      vtkErrorMacro("ExecuteInformation: No LookupTable was set but input data is not VTK_UNSIGNED_CHAR, therefore input can't be passed through!");
      return;
      }
    else if (numComponents != inData->GetNumberOfScalarComponents())
      {
      vtkErrorMacro("ExecuteInformation: No LookupTable was set but number of components in input doesn't match OutputFormat, therefore input can't be passed through!");
      return;
      }
    }
  

  outData->SetScalarType(VTK_UNSIGNED_CHAR);
  outData->SetNumberOfScalarComponents(numComponents);
}

//----------------------------------------------------------------------------
// This non-templated function executes the filter for any type of data.

static void vtkImageMapToColorsExecute(vtkImageMapToColors *self,
				       vtkImageData *inData, void *inPtr,
				       vtkImageData *outData, 
				       unsigned char *outPtr,
				       int outExt[6], int id)
{
  int idxY, idxZ;
  int extX, extY, extZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int dataType = inData->GetScalarType();
  int scalarSize = inData->GetScalarSize();
  int numberOfComponents,numberOfOutputComponents,outputFormat;
  int rowLength;
  vtkScalarsToColors *lookupTable = self->GetLookupTable();
  unsigned char *outPtr1;
  void *inPtr1;

  // find the region to loop over
  extX = outExt[1] - outExt[0] + 1;
  extY = outExt[3] - outExt[2] + 1; 
  extZ = outExt[5] - outExt[4] + 1;

  target = (unsigned long)(extZ*extY/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  // because we are using void * and char * we must take care
  // of the scalar size in the increments
  inIncY *= scalarSize;
  inIncZ *= scalarSize;
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  numberOfComponents = inData->GetNumberOfScalarComponents();
  numberOfOutputComponents = outData->GetNumberOfScalarComponents();
  outputFormat = self->GetOutputFormat();
  rowLength = extX*scalarSize*numberOfComponents;

  // Loop through output pixels
  outPtr1 = outPtr;
  inPtr1 = (void *) ((char *) inPtr + self->GetActiveComponent()*scalarSize);
  for (idxZ = 0; idxZ < extZ; idxZ++)
    {
    for (idxY = 0; !self->AbortExecute && idxY < extY; idxY++)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      lookupTable->MapScalarsThroughTable2(inPtr1,outPtr1,
					   dataType,extX,numberOfComponents,
					   outputFormat);
      if (self->GetPassAlphaToOutput() && 
	  dataType == VTK_UNSIGNED_CHAR && numberOfComponents > 1 &&
	  (outputFormat == VTK_RGBA || outputFormat == VTK_LUMINANCE_ALPHA))
	{
	unsigned char *outPtr2 = outPtr1 + numberOfOutputComponents - 1;
	unsigned char *inPtr2 = (unsigned char *)inPtr1
	                              - self->GetActiveComponent()*scalarSize
	                              + numberOfComponents - 1;
	for (int i = 0; i < extX; i++)
	  {
	  *outPtr2 = (*outPtr2 * *inPtr2)/255;
	  outPtr2 += numberOfOutputComponents;
	  inPtr2 += numberOfComponents;
	  }
	}
      outPtr1 += outIncY + extX*numberOfOutputComponents;
      inPtr1 = (void *) ((char *) inPtr1 + inIncY + rowLength);
      }
    outPtr1 += outIncZ;
    inPtr1 = (void *) ((char *) inPtr1 + inIncZ);
    }
}

//----------------------------------------------------------------------------
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.

void vtkImageMapToColors::ThreadedExecute(vtkImageData *inData, 
					 vtkImageData *outData,
					 int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkImageMapToColorsExecute(this, inData, inPtr, 
			   outData, (unsigned char *)outPtr, outExt, id);
}

void vtkImageMapToColors::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "OutputFormat: " << 
    (this->OutputFormat == VTK_RGBA ? "RGBA" : 
     (this->OutputFormat == VTK_RGB ? "RGB" :
      (this->OutputFormat == VTK_LUMINANCE_ALPHA ? "LuminanceAlpha" :
       (this->OutputFormat == VTK_LUMINANCE ? "Luminance" : "Unknown"))))
     << "\n";
  os << indent << "ActiveComponent: " << this->ActiveComponent << "\n";
  os << indent << "PassAlphaToOutput: " << this->PassAlphaToOutput << "\n";
  os << indent << "LookupTable: " << this->LookupTable << "\n";
  if (this->LookupTable)
    {
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
}





