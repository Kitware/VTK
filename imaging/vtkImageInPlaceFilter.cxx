/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageInPlaceFilter.cxx
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

#include "vtkImageInPlaceFilter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageInPlaceFilter* vtkImageInPlaceFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageInPlaceFilter");
  if(ret)
    {
    return (vtkImageInPlaceFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageInPlaceFilter;
}



//----------------------------------------------------------------------------

void vtkImageInPlaceFilter::Execute()
{
  vtkImageData *output = this->GetOutput();
  int *inExt, *outExt;

  output->SetExtent(output->GetUpdateExtent());
  output->AllocateScalars();

  inExt = this->GetInput()->GetUpdateExtent();
  outExt = this->GetOutput()->GetUpdateExtent();

  vtkImageData *input;
  input = this->GetInput();

  if ((inExt[0] == outExt[0])&&(inExt[1] == outExt[1])&&
      (inExt[2] == outExt[2])&&(inExt[3] == outExt[3])&&
      (inExt[4] == outExt[4])&&(inExt[5] == outExt[5])&&
      this->GetInput()->ShouldIReleaseData())
    {
    // pass the data
    output->GetPointData()->PassData(input->GetPointData());
    }
  else
    {
    this->CopyData(input,output);
    }
    
  this->Execute(input, output);
}
  
  

void vtkImageInPlaceFilter::CopyData(vtkImageData *inData,
				     vtkImageData *outData)
{
  int *outExt = this->GetOutput()->GetUpdateExtent();
  char *inPtr = (char *) inData->GetScalarPointerForExtent(outExt);
  char *outPtr = (char *) outData->GetScalarPointerForExtent(outExt);
  int rowLength, size;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int idxY, idxZ, maxY, maxZ;
  
  rowLength = (outExt[1] - outExt[0]+1)*inData->GetNumberOfScalarComponents();
  size = inData->GetScalarSize();
  rowLength *= size;
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  
  // adjust increments for this loop
  inIncY = inIncY*size + rowLength;
  outIncY = outIncY*size + rowLength;
  inIncZ *= size;
  outIncZ *= size;
  
  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      memcpy(outPtr,inPtr,rowLength);
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}
