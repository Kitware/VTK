/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToRGBA.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in indvidual files. This copyright specifically does
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
#include "vtkImageMapToRGBA.h"



//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageMapToRGBA::vtkImageMapToRGBA()
{
  this->LookupTable = NULL;
}


vtkImageMapToRGBA::~vtkImageMapToRGBA()
{
  if (this->LookupTable != NULL) 
    {
    this->LookupTable->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
void vtkImageMapToRGBA::ExecuteInformation()
{
  if (this->LookupTable == 0)
    {
    vtkErrorMacro(<< "ExecuteInformation: No LookupTable was set!");
    }
  this->GetOutput()->SetScalarType(VTK_UNSIGNED_CHAR);
  this->GetOutput()->SetNumberOfScalarComponents(4);
}

//----------------------------------------------------------------------------
// This non-templated function executes the filter for any type of data.

static void vtkImageMapToRGBAExecute(vtkImageMapToRGBA *self,
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
  int numberOfComponents;
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
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  numberOfComponents = inData->GetNumberOfScalarComponents();
  rowLength = extX*scalarSize;

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
      lookupTable->MapScalarsThroughTable2(inPtr1,(unsigned char *)outPtr1,
					  dataType,extX,numberOfComponents);
      outPtr1 += outIncY + extX*4;
      inPtr1 = (void *) ((char *) inPtr1 + inIncY + rowLength);
      }
    outPtr1 += outIncZ;
    inPtr1 = (void *) ((char *) inPtr1 + inIncZ);
    }
}

//----------------------------------------------------------------------------
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.

void vtkImageMapToRGBA::ThreadedExecute(vtkImageData *inData, 
					 vtkImageData *outData,
					 int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkImageMapToRGBAExecute(this, inData, inPtr, 
			   outData, (unsigned char *)outPtr, outExt, id);
}

void vtkImageMapToRGBA::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "LookupTable: " << this->LookupTable << "\n";
  if (this->LookupTable)
    {
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
}





