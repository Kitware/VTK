/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToWindowLevelColors.cxx
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
void vtkImageMapToWindowLevelColors::ExecuteData(vtkDataObject *output)
{
  vtkImageData *outData = (vtkImageData *)(output);
  vtkImageData *inData = this->GetInput();
 
  // If LookupTable is null and window / level produces no change,
  // then just pass the data
  if (this->LookupTable == NULL &&
      (inData->GetScalarType() == VTK_UNSIGNED_CHAR &&
       this->Window == 255 && this->Level == 127.5))
    {
    vtkDebugMacro("ExecuteData: LookupTable not set, "\
		  "Window / Level at default, "\
		  "passing input to output.");

    outData->SetExtent(inData->GetExtent());
    outData->GetPointData()->PassData(inData->GetPointData());
    this->DataWasPassed = 1;
    }
  else
    // normal behaviour - skip up a level since we don't want to
    // call the superclasses ExecuteData - it would pass the data if there
    // is no lookup table even if there is a window / level - wrong
    // behavior.
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

/* 
 * This templated routine calculates effective lower and upper limits 
 * for a window of values of type T, lower and upper. 
 */
template <class T>
static void vtkImageMapToWindowLevelClamps ( vtkImageData *data, float w, 
                                             float l, T& lower, T& upper, 
                                             unsigned char &lower_val, 
                                             unsigned char &upper_val)
{
  double f_lower, f_upper, f_lower_val, f_upper_val;
  double adjustedLower, adjustedUpper;
  double range[2];

  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  f_lower = l - fabs(w) / 2.0;
  f_upper = f_lower + fabs(w);

  // Set the correct lower value
  if ( f_lower <= range[1])
    {
    if (f_lower >= range[0])
      {
      lower = (T) f_lower;
      adjustedLower = f_lower;
      }
    else
      {
      lower = (T) range[0];
      adjustedLower = range[0];
      }
    }
  else
    {
    lower = (T) range[1];
    adjustedLower = range[1];
    }
  
  
  // Set the correct upper value
  if ( f_upper >= range[0])
    {
    if (f_upper <= range[1])
      {
      upper = (T) f_upper;
      adjustedUpper = f_upper;
      }
    else
      {
      upper = (T) range[1];
      adjustedUpper = range[1];
      }
    }
  else
    {
    upper = (T) range [0];
    adjustedUpper = range [0];
    }
  
  // now compute the lower and upper values
  if (w >= 0)
    {
    f_lower_val = 255.0*(adjustedLower - f_lower)/w;
    f_upper_val = 255.0*(adjustedUpper - f_lower)/w;
    }
  else
    {
    f_lower_val = 255.0 + 255.0*(adjustedLower - f_lower)/w;
    f_upper_val = 255.0 + 255.0*(adjustedUpper - f_lower)/w;
    }
  
  if (f_upper_val > 255) 
    {
    upper_val = 255;
    }
  else if (f_upper_val < 0)
    {
    upper_val = 0;
    }
  else
    {
    upper_val = (unsigned char)(f_upper_val);
    }
  
  if (f_lower_val > 255) 
    {
    lower_val = 255;
    }
  else if (f_lower_val < 0)
    {
    lower_val = 0;
    }
  else
    {
    lower_val = (unsigned char)(f_lower_val);
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
  int numberOfComponents,numberOfOutputComponents,outputFormat;
  int rowLength;
  vtkScalarsToColors *lookupTable = self->GetLookupTable();
  unsigned char *outPtr1;
  T *inPtr1;
  unsigned char *optr;
  T    *iptr;
  float shift =  self->GetWindow() / 2.0 - self->GetLevel();
  float scale = 255.0 / self->GetWindow();

  T   lower, upper;
  unsigned char lower_val, upper_val, result_val;
  unsigned short ushort_val;
  vtkImageMapToWindowLevelClamps( inData, self->GetWindow(), 
                                  self->GetLevel(), 
                                  lower, upper, lower_val, upper_val );
  
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
          if (*iptr <= lower) 
            {
            ushort_val = lower_val;
            }
          else if (*iptr >= upper)
            {
            ushort_val = upper_val;
            }
          else
            {
            ushort_val = (unsigned char) ((*iptr + shift)*scale);
            }
          *optr = (unsigned char)((*optr * ushort_val) >> 8);
          switch (outputFormat)
            {
            case VTK_RGBA:
              *(optr+1) = (unsigned char)((*(optr+1) * ushort_val) >> 8);
              *(optr+2) = (unsigned char)((*(optr+2) * ushort_val) >> 8);
              *(optr+3) = 255;
              break;
            case VTK_RGB:
              *(optr+1) = (unsigned char)((*(optr+1) * ushort_val) >> 8);
              *(optr+2) = (unsigned char)((*(optr+2) * ushort_val) >> 8);
              break;
            case VTK_LUMINANCE_ALPHA:
              *(optr+1) = 255;
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
          if (*iptr <= lower) 
            {
            result_val = lower_val;
            }
          else if (*iptr >= upper)
            {
            result_val = upper_val;
            }
          else
            {
            result_val = (unsigned char) ((*iptr + shift)*scale);
            }
          *optr = result_val;
          switch (outputFormat)
            {
            case VTK_RGBA:
              *(optr+1) = result_val;
              *(optr+2) = result_val;            
              *(optr+3) = 255;
              break;
            case VTK_RGB:
              *(optr+1) = result_val;
              *(optr+2) = result_val;            
              break;
            case VTK_LUMINANCE_ALPHA:
              *(optr+1) = 255;
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





