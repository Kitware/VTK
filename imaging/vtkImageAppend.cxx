/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppend.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkImageCache.h"
#include "vtkImageAppend.h"


//----------------------------------------------------------------------------
vtkImageAppend::vtkImageAppend()
{
  this->AppendAxis = 0;
  this->Shifts = NULL;
}

//----------------------------------------------------------------------------
vtkImageAppend::~vtkImageAppend()
{
  if (this->Shifts != NULL)
    {
    delete [] this->Shifts;
    }
}



//----------------------------------------------------------------------------
// This method tells the ouput it will have more components
void vtkImageAppend::ExecuteImageInformation()
{
  int idx;
  int min, max, size, tmp;
  int *inExt, outExt[6];

  if (this->Inputs[0] == NULL)
    {
    vtkErrorMacro("No input");
    return;
    }
  
  if (this->Shifts)
    {
    delete [] this->Shifts;
    }
  this->Shifts = new int [this->NumberOfInputs];
  
  // Find the outMin/max of the appended axis for this input.
  inExt = this->Inputs[0]->GetWholeExtent();
  min = tmp = inExt[this->AppendAxis * 2];
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      inExt = this->Inputs[idx]->GetWholeExtent();
      this->Shifts[idx] = tmp - inExt[this->AppendAxis*2];
      size = inExt[this->AppendAxis*2 + 1] - inExt[this->AppendAxis*2] + 1;
      tmp += size;
      }
    }
  max = tmp - 1;
  
  this->Inputs[0]->GetWholeExtent(outExt);
  outExt[this->AppendAxis*2] = min;
  outExt[this->AppendAxis*2 + 1] = max;

  this->Output->SetWholeExtent(outExt);
}


//----------------------------------------------------------------------------
void vtkImageAppend::ComputeRequiredInputUpdateExtent(int inExt[6],
					      int outExt[6], int whichInput)
{
  int min, max, shift, tmp, idx;
  int *extent;

  if (this->Inputs[0] == NULL)
    {
    vtkErrorMacro("No input");
    return;
    }
  
  // default input extent will be that of output extent
  memcpy(inExt,outExt,sizeof(int)*6);

  // Find the outMin/max of the appended axis for this input.
  extent = this->Inputs[whichInput]->GetWholeExtent();
  shift = this->Shifts[whichInput];
  min = extent[this->AppendAxis*2] + shift;
  max = extent[this->AppendAxis*2 + 1] + shift;
  
  // now clip the outExtent against the outExtent for this input (intersect)
  tmp = outExt[this->AppendAxis*2];
  if (min < tmp) {min = tmp;}
  tmp = outExt[this->AppendAxis*2 + 1];
  if (max > tmp) {max = tmp;}
  
  // now if min > max, we do not need the input at all.  I assume
  // the pipeline will interpret this extent this way.
  
  // convert back into input coordinates.
  inExt[this->AppendAxis*2] = min - shift;
  inExt[this->AppendAxis*2 + 1] = max - shift;
  
  // for robustness (in the execute method), 
  // do not ask for more than the whole extent of the other axes.
  for (idx = 0; idx < 3; ++idx)
    {
    if (inExt[idx*2] < extent[idx*2])
      {
      inExt[idx*2] = extent[idx*2];
      }
    if (inExt[idx*2 + 1] > extent[idx*2 + 1])
      {
      inExt[idx*2 + 1] = extent[idx*2 + 1];
      }
    }
}



//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageAppendExecute(vtkImageAppend *self, int id, 
			  int inExt[6], vtkImageData *inData, T *inPtr,
			  int outExt[6], vtkImageData *outData, T *outPtr)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;

  // find the region to loop over
  rowLength = (inExt[1] - inExt[0]+1)*inData->GetNumberOfScalarComponents();
  maxY = inExt[3] - inExt[2]; 
  maxZ = inExt[5] - inExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(inExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      for (idxR = 0; idxR < rowLength; idxR++)
	{
	// Pixel operation
	*outPtr = *inPtr;
	outPtr++;
	inPtr++;
	}
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}

//----------------------------------------------------------------------------
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageAppend::ThreadedExecute(vtkImageData **inData, 
				     vtkImageData *outData,
				     int outExt[6], int id)
{
  int idx1;
  int inExt[6], cOutExt[6];
  void *inPtr;
  void *outPtr;
  
  for (idx1 = 0; idx1 < this->NumberOfInputs; ++idx1)
    {
    if (inData[idx1] != NULL)
      {
      // Get the input extent and output extent
      // the real out extent for this input may be clipped.
      memcpy(inExt, outExt, 6*sizeof(int));
      this->ComputeRequiredInputUpdateExtent(inExt, outExt, idx1);
      memcpy(cOutExt, inExt, 6*sizeof(int));
      cOutExt[this->AppendAxis*2] = 
	inExt[this->AppendAxis*2] + this->Shifts[idx1];
      cOutExt[this->AppendAxis*2 + 1] = 
	inExt[this->AppendAxis*2 + 1] + this->Shifts[idx1];
      
      // doo a quick check to see if the input is used at all.
      if (inExt[this->AppendAxis*2] <= inExt[this->AppendAxis*2 + 1])
	{
	inPtr = inData[idx1]->GetScalarPointerForExtent(inExt);
	outPtr = outData->GetScalarPointerForExtent(cOutExt);

	if (inData[idx1]->GetNumberOfScalarComponents() !=
	    outData->GetNumberOfScalarComponents())
	  {
	  vtkErrorMacro("Components of the inputs do not match");
	  return;
	  }
	
	// this filter expects that input is the same type as output.
	if (inData[idx1]->GetScalarType() != outData->GetScalarType())
	  {
	  vtkErrorMacro(<< "Execute: input" << idx1 << " ScalarType (" << 
	  inData[idx1]->GetScalarType() << 
	  "), must match output ScalarType (" << outData->GetScalarType() 
	  << ")");
	  return;
	  }
	
	switch (inData[idx1]->GetScalarType())
	  {
	  case VTK_FLOAT:
	    vtkImageAppendExecute(this, id, inExt,
				  inData[idx1], (float *)(inPtr),
				  cOutExt, outData, (float *)(outPtr));
	    break;
	  case VTK_INT:
	    vtkImageAppendExecute(this, id, inExt,
				  inData[idx1], (int *)(inPtr),
				  cOutExt, outData, (int *)(outPtr));
	    break;
	  case VTK_SHORT:
	    vtkImageAppendExecute(this, id, inExt,
				  inData[idx1], (short *)(inPtr),
				  cOutExt, outData, (short *)(outPtr));
	    break;
	  case VTK_UNSIGNED_SHORT:
	    vtkImageAppendExecute(this, id, inExt,
				  inData[idx1], (unsigned short *)(inPtr),
				  cOutExt, outData,(unsigned short *)(outPtr));
	    break;
	  case VTK_UNSIGNED_CHAR:
	    vtkImageAppendExecute(this, id, inExt,
				  inData[idx1], (unsigned char *)(inPtr),
				  cOutExt, outData, (unsigned char *)(outPtr));
	    break;
	  default:
	    vtkErrorMacro(<< "Execute: Unknown ScalarType");
	    return;
	  }
	}
      }
    }
  
}



//----------------------------------------------------------------------------
void vtkImageAppend::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageMultipleInputFilter::PrintSelf(os, indent);
  os << indent << "AppendAxis: " << this->AppendAxis << endl;
}














