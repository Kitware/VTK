/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToWindowLevelColors.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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
#include "vtkImageMapToWindowLevelColors.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkImageMapToWindowLevelColors* vtkImageMapToWindowLevelColors::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageMapToWindowLevelColors");
  if(ret)
    {
    return (vtkImageMapToWindowLevelColors*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageMapToWindowLevelColors;
}

//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageMapToWindowLevelColors::vtkImageMapToWindowLevelColors()
{
  this->Window = 255;
  this->Level  = 127.5;
}


vtkImageMapToWindowLevelColors::~vtkImageMapToWindowLevelColors()
{
}

//----------------------------------------------------------------------------
// This method checks to see if we can simply reference the input data
void vtkImageMapToWindowLevelColors::UpdateData(vtkDataObject *outObject)
{
  vtkImageData *outData = (vtkImageData *)(outObject);
  vtkImageData *inData = this->GetInput();
  
  // If LookupTable is null and window / level produces no change,
  // then just pass the data
  if ( this->LookupTable == NULL &&
       (inData->GetScalarType() == VTK_UNSIGNED_CHAR &&
          this->Window == 255 && this->Level == 127.5) )
    {
    vtkDebugMacro("UpdateData: LookupTable not set " << 
                  "Window / Level at default, passing input to output.");

    inData->SetUpdateExtent(outData->GetUpdateExtent());
    inData->Update();
    outData->SetExtent(inData->GetExtent());
    outData->GetPointData()->PassData(inData->GetPointData());
    outData->DataHasBeenGenerated();
    this->DataWasPassed = 1;
    }
  else 
    // normal behaviour - skip up a level since we don't want to
    // call the superclasses UpdateData - it would pass the data if there
    // is no lookup table even if there is a window / level - wrong
    // behavior.
    {
    if ( this->DataWasPassed )
      {
      outData->GetPointData()->SetScalars(NULL);
      this->DataWasPassed = 0;
      }
    
    this->vtkImageToImageFilter::UpdateData(outObject);
    }
}

//----------------------------------------------------------------------------
void vtkImageMapToWindowLevelColors::ExecuteInformation(vtkImageData *inData, 
					     vtkImageData *outData)
{
  // If LookupTable is null and window / level produces no change,
  // then the data will be passed
  if ( this->LookupTable == NULL &&
       ( (inData->GetScalarType() == VTK_UNSIGNED_CHAR &&
          this->Window == 255 && this->Level == 127.5) ||
         (inData->GetScalarType() == VTK_UNSIGNED_CHAR &&
          this->Window == 255 && this->Level == 127.5) ) )
    {
    if (inData->GetScalarType() != VTK_UNSIGNED_CHAR)
      {
      vtkErrorMacro("ExecuteInformation: No LookupTable was set and input data is not VTK_UNSIGNED_CHAR!");
      }
    else
      {
      // no lookup table, pass the input if it was UNSIGNED_CHAR 
      outData->SetScalarType(VTK_UNSIGNED_CHAR);
      outData->SetNumberOfScalarComponents(
			      inData->GetNumberOfScalarComponents());
      }
    }
  else  // the lookup table was set or window / level produces a change
    {
    int numComponents = 4;
    outData->SetScalarType(VTK_UNSIGNED_CHAR);
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
    outData->SetNumberOfScalarComponents(numComponents);
    }
}

//----------------------------------------------------------------------------
// This non-templated function executes the filter for any type of data.
template <class T>
static void vtkImageMapToWindowLevelColorsExecute(vtkImageMapToWindowLevelColors *self,
				       vtkImageData *inData, T *inPtr,
				       vtkImageData *outData, 
				       unsigned char *outPtr,
				       int outExt[6], int id)
{
  int idxX, idxY, idxZ;
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
  T *inPtr1;
  unsigned char *optr;
  T    *iptr;
  float min = self->GetLevel() - self->GetWindow() / 2.0;
  float width = self->GetWindow();
  float value;
  
  
  // find the region to loop over
  extX = outExt[1] - outExt[0] + 1;
  extY = outExt[3] - outExt[2] + 1; 
  extZ = outExt[5] - outExt[4] + 1;

  target = (unsigned long)(extZ*extY/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);

  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  numberOfComponents = inData->GetNumberOfScalarComponents();
  numberOfOutputComponents = outData->GetNumberOfScalarComponents();
  outputFormat = self->GetOutputFormat();
  
  rowLength = extX;

  // Loop through output pixels
  outPtr1 = outPtr;
  inPtr1 = inPtr;
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
      
      iptr = inPtr1;
      optr = outPtr1;
      
      if ( lookupTable )
        {
        lookupTable->MapScalarsThroughTable2(inPtr1,(unsigned char *)outPtr1,
                                             dataType,extX,numberOfComponents,
                                             outputFormat);
      
        for (idxX = 0; idxX < extX; idxX++)
          {
          value = ((float)(*iptr) - min) / width;
          value = (value < 0.0)?(0.0):(value);
          value = (value > 1.0)?(1.0):(value);
          
          switch (outputFormat)
            {
            case VTK_RGBA:
              *(optr  ) = (unsigned char)((float)*(optr  ) * value);
              *(optr+1) = (unsigned char)((float)*(optr+1) * value);
              *(optr+2) = (unsigned char)((float)*(optr+2) * value);
              *(optr+3) = 255;
              break;
            case VTK_RGB:
              *(optr  ) = (unsigned char)((float)*(optr  ) * value);
              *(optr+1) = (unsigned char)((float)*(optr+1) * value);
              *(optr+2) = (unsigned char)((float)*(optr+2) * value);
              break;
            case VTK_LUMINANCE_ALPHA:
              *optr     = (unsigned char)((float)*optr * value);
              *(optr+1) = 255;
              break;
            case VTK_LUMINANCE:
              *optr     = (unsigned char)((float)*optr * value);
              break;
            }
          iptr++;
          optr += numberOfOutputComponents;
          }
        }
      else
        {
        for (idxX = 0; idxX < extX; idxX++)
          {
          value = ((float)(*iptr) - min) / width;
          value = (value < 0.0)?(0.0):(value);
          value = (value > 1.0)?(1.0):(value);
          
          switch (outputFormat)
            {
            case VTK_RGBA:
              *(optr  ) = (unsigned char)(255.0 * value);
              *(optr+1) = (unsigned char)(255.0 * value);
              *(optr+2) = (unsigned char)(255.0 * value);            
              *(optr+3) = 255;
              break;
            case VTK_RGB:
              *(optr  ) = (unsigned char)(255.0 * value);
              *(optr+1) = (unsigned char)(255.0 * value);
              *(optr+2) = (unsigned char)(255.0 * value);            
              break;
            case VTK_LUMINANCE_ALPHA:
              *optr     = (unsigned char)(255.0 * value);
              *(optr+1) = 255;
              break;
            case VTK_LUMINANCE:
              *optr     = (unsigned char)(255.0 * value);
              break;
            }
          
          iptr++;
          optr += numberOfOutputComponents;
          }
        }
      
      outPtr1 += outIncY + extX*numberOfOutputComponents;
      inPtr1 += inIncY + rowLength;
      }
    outPtr1 += outIncZ;
    inPtr1 += inIncZ;
    }
}

//----------------------------------------------------------------------------
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.

void vtkImageMapToWindowLevelColors::ThreadedExecute(vtkImageData *inData, 
					 vtkImageData *outData,
					 int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro7(vtkImageMapToWindowLevelColorsExecute, this, 
                      inData, (VTK_TT *)(inPtr), 
                      outData, (unsigned char *)(outPtr), outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

void vtkImageMapToWindowLevelColors::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageMapToColors::PrintSelf(os,indent);

  os << indent << "Window: " << this->Window << endl;
  os << indent << "Level: " << this->Level << endl;
}





