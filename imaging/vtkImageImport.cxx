/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageImport.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkByteSwap.h"
#include "vtkImageImport.h"

//----------------------------------------------------------------------------
vtkImageImport::vtkImageImport()
{
  int idx;
  
  this->ImportVoidPointer = 0;

  this->DataScalarType = VTK_SHORT;
  this->NumberOfScalarComponents = 1;
  
  for (idx = 0; idx < 3; ++idx)
    {
    this->DataExtent[idx*2] = this->DataExtent[idx*2 + 1] = 0;
    this->DataSpacing[idx] = 1.0;
    this->DataOrigin[idx] = 0.0;
    }
  
  this->ImageLowerLeft = 1;
}

//----------------------------------------------------------------------------
vtkImageImport::~vtkImageImport()
{ 
}

//----------------------------------------------------------------------------
void vtkImageImport::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkImageSource::PrintSelf(os,indent);

  os << indent << "ImportVoidPointer: " << this->ImportVoidPointer << "\n";

  os << indent << "DataScalarType: " 
     << vtkImageScalarTypeNameMacro(this->DataScalarType) << "\n";

  os << indent << "NumberOfScalarComponents: " 
     << this->NumberOfScalarComponents << "\n";
 
  os << indent << "DataExtent: (" << this->DataExtent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->DataExtent[idx];
    }
  os << ")\n";
  
  os << indent << "DataSpacing: (" << this->DataSpacing[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->DataSpacing[idx];
    }
  os << ")\n";
  
  os << indent << "DataOrigin: (" << this->DataOrigin[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->DataOrigin[idx];
    }
  os << ")\n";

  os << indent << "ImageLowerLeft: " << 
    (this->ImageLowerLeft ? "On\n" : "Off\n");

}


//----------------------------------------------------------------------------
// This method returns the largest data that can be generated.
void vtkImageImport::UpdateImageInformation()
{
  // set the extent
  this->GetOutput()->SetWholeExtent(this->DataExtent);
    
  // set the spacing
  this->GetOutput()->SetSpacing(this->DataSpacing);

  // set the origin.
  this->GetOutput()->SetOrigin(this->DataOrigin);

  // set data type
  this->GetOutput()->SetScalarType(this->DataScalarType);
  this->GetOutput()->
    SetNumberOfScalarComponents(this->NumberOfScalarComponents);
}

// A templated function to import the data and copy it into the output.
template<class T>
void vtkImageImportExecute(vtkImageImport *self, vtkImageData *data, T *inPtr)
{
  int i,j;

  int outputExtent[6];
  int inputExtent[6];
  data->GetExtent(outputExtent);
  self->GetDataExtent(inputExtent);

  int inExtX = inputExtent[1]-inputExtent[0]+1;
  int inExtY = inputExtent[3]-inputExtent[2]+1;
  int inExtZ = inputExtent[5]-inputExtent[4]+1;

  int outExtX = outputExtent[1]-outputExtent[0]+1;
  int outExtY = outputExtent[3]-outputExtent[2]+1;
  int outExtZ = outputExtent[5]-outputExtent[4]+1;

  T *outPtr = (T *)data->GetScalarPointer();
  T *inPtrTmp,*outPtrTmp;

  int inIncX = self->GetNumberOfScalarComponents();
  int inIncY = inIncX*inExtX;
  int inIncZ = inIncY*inExtY;

  int outIncX, outIncY, outIncZ;
  data->GetIncrements(outIncX, outIncY, outIncZ);

  if (!self->GetImageLowerLeft())
    { // apply a vertical flip while copying to output
    for (i = 0; i < outExtZ; i++)
      {
      inPtrTmp = inPtr;
      outPtrTmp = outPtr + outIncY*outExtY;
      for (j = 0; j < outExtY; j++)
	{
	outPtrTmp -= outIncY;
	memcpy(outPtrTmp,inPtrTmp,outExtX*outIncX*sizeof(T));
	inPtrTmp += inIncY;
	}
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else
    { // don't apply a vertical flip
    for (i = 0; i < outExtZ; i++)
      {
      inPtrTmp = inPtr;
      outPtrTmp = outPtr;
      for (j = 0; j < outExtY; j++)
	{
	memcpy(outPtrTmp,inPtrTmp,outExtX*outIncX*sizeof(T));
	outPtrTmp += outIncY;
	inPtrTmp += inIncY;
	}
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
}

//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkImageImport::Execute(vtkImageData *data)
{
  void *ptr = this->GetImportVoidPointer();
  
  // Call the correct templated function for the output
  switch (this->GetDataScalarType())
    {
    case VTK_DOUBLE:
      vtkImageImportExecute(this, data, (double *)(ptr));
      break;
    case VTK_FLOAT:
      vtkImageImportExecute(this, data, (float *)(ptr));
      break;
    case VTK_INT:
      vtkImageImportExecute(this, data, (int *)(ptr));
      break;
    case VTK_SHORT:
      vtkImageImportExecute(this, data, (short *)(ptr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageImportExecute(this, data, (unsigned short *)(ptr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageImportExecute(this, data, (unsigned char *)(ptr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown data type");
    }   
}


