/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageExport.cxx
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
#include <ctype.h>
#include <string.h>
#include "vtkImageExport.h"



//----------------------------------------------------------------------------
vtkImageExport::vtkImageExport()
{
  this->Input = NULL;
  this->ImageFlip = NULL;
  this->ImageLowerLeft = 1;
}



//----------------------------------------------------------------------------
vtkImageExport::~vtkImageExport()
{
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
  if (this->ImageFlip)
    {
    this->ImageFlip->UnRegister(this);
    this->ImageFlip = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkImageExport::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkProcessObject::PrintSelf(os,indent);

  os << indent << "Input: (" << this->Input << ")\n";

  os << indent << "ImageLowerLeft: " 
     << (this->ImageLowerLeft ? "On\n" : "Off\n");
}


//----------------------------------------------------------------------------
int vtkImageExport::GetDataMemorySize()
{
  int size;
  this->Input->UpdateImageInformation();
  int *extent = this->GetInput()->GetWholeExtent();

  // take into consideration the scalar type
  switch (this->GetInput()->GetScalarType())
    {
    case VTK_DOUBLE:
      size = sizeof(double);
      break;
    case VTK_FLOAT:
      size = sizeof(float);
      break;
    case VTK_INT:
      size = sizeof(int);
      break;
    case VTK_SHORT:
      size = sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      size = sizeof(unsigned short); 
      break;
    case VTK_UNSIGNED_CHAR:
      size = sizeof(unsigned char); 
      break;
    default:
      vtkErrorMacro(<< "GetDataMemorySize: Unknown output ScalarType.");
      return 0; 
    }
  size *= this->GetInput()->GetNumberOfScalarComponents();
  size *= (extent[1] - extent[0] + 1);
  size *= (extent[3] - extent[2] + 1);
  size *= (extent[5] - extent[4] + 1);

  return size;
}


//----------------------------------------------------------------------------
void vtkImageExport::GetDataDimensions(int *dims)
{
  this->Input->UpdateImageInformation();
  int *extent = this->GetInput()->GetWholeExtent();
  dims[0] = extent[1]-extent[0]+1;
  dims[1] = extent[3]-extent[2]+1;
  dims[2] = extent[5]-extent[4]+1;
}

//----------------------------------------------------------------------------
// Exports a region in a file.  Subclasses can override this method
// to produce a header. This method only hanldes 3d data (plus components).
void vtkImageExport::FinalExport(vtkImageData *data, int extent[6],
				 void **output)
{
  int idxY, idxZ;
  int rowLength; // in bytes
  void *ptr;
  
  // Make sure we actually have data.
  if ( !data->GetPointData()->GetScalars())
    {
    vtkErrorMacro(<< "Could not get data from input.");
    return;
    }

  // take into consideration the scalar type
  switch (data->GetScalarType())
    {
    case VTK_DOUBLE:
      rowLength = sizeof(double);
      break;
    case VTK_FLOAT:
      rowLength = sizeof(float);
      break;
    case VTK_INT:
      rowLength = sizeof(int);
      break;
    case VTK_SHORT:
      rowLength = sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      rowLength = sizeof(unsigned short); 
      break;
    case VTK_UNSIGNED_CHAR:
      rowLength = sizeof(unsigned char); 
      break;
    default:
      vtkErrorMacro(<< "Export: Unknown output ScalarType.");
      return; 
    }
  rowLength *= data->GetNumberOfScalarComponents();
  rowLength *= (extent[1] - extent[0] + 1);

  int *wExtent = this->GetInput()->GetWholeExtent();
  float area = (float) ((extent[5] - extent[4] + 1)*
			(extent[3] - extent[2] + 1)*
			(extent[1] - extent[0] + 1)) / 
               (float) ((wExtent[5] -wExtent[4] + 1)*
			(wExtent[3] -wExtent[2] + 1)*
			(wExtent[1] -wExtent[0] + 1));
    
  unsigned long count = 0;
  unsigned long target = (unsigned long)((extent[5]-extent[4]+1)*
			   (extent[3]-extent[2]+1)/(50.0*area));
  target++;

  if (!this->GetImageLowerLeft())
    { // flip the image
    for (idxZ = extent[4]; idxZ <= extent[5]; ++idxZ)
      {
      for (idxY = extent[3]; idxY >= extent[2]; idxY--)
	{
	if (!(count%target))
	  {
	  this->UpdateProgress(this->GetProgress() + count/(50.0*target));
	  }
	count++;
	ptr = data->GetScalarPointer(extent[0], idxY, idxZ);
	memcpy(*output,(void *)ptr,rowLength);
	*output = (void *)(((char *)*output) + rowLength);
	}
      }
    }
  else
    { // don't flip the image
    for (idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (idxY = extent[2]; idxY <= extent[3]; idxY++)
	{
	if (!(count%target))
	  {
	  this->UpdateProgress(this->GetProgress() + count/(50.0*target));
	  }
	count++;
	ptr = data->GetScalarPointer(extent[0], idxY, idxZ);
	memcpy(*output,(void *)ptr,rowLength);
	*output = (void *)(((char *)*output) + rowLength);
	}
      }
    }
}

//----------------------------------------------------------------------------
// Breaks region into pieces with correct dimensionality.
void vtkImageExport::RecursiveExport(int axis, vtkImageCache *cache,
				     void **output)
{
  int min, max, mid;
  vtkImageData *data;
  
  if (cache->GetUpdateExtentMemorySize() < cache->GetMemoryLimit())
    {
    data = cache->UpdateAndReturnData();
    this->FinalExport(data,cache->GetUpdateExtent(),output);
    return;
    }

  // if the current request did not fit into memory
  // the we will split the current axis
  cache->GetAxisUpdateExtent(axis, min, max);
  if (min == max)
    {
    if (axis > 0)
      {
      this->RecursiveExport(axis-1, cache, output);
      }
    else
      {
      vtkWarningMacro("Cache too small to hold one row of pixels!!");
      }
    return;
    }
  
  mid = (min + max) / 2;

  // if it is the y axis then flip by default
  if (axis == 1 && !this->ImageLowerLeft)
    {
    // first half
    cache->SetAxisUpdateExtent(axis, mid+1, max);
    this->RecursiveExport(axis,cache,output);
    
    // second half
    cache->SetAxisUpdateExtent(axis, min, mid);
    this->RecursiveExport(axis,cache,output);
    }
  else
    {
    // first half
    cache->SetAxisUpdateExtent(axis, min, mid);
    this->RecursiveExport(axis,cache,output);
    
    // second half
    cache->SetAxisUpdateExtent(axis, mid+1, max);
    this->RecursiveExport(axis,cache,output);
    }
    
  // restore original extent
  cache->SetAxisUpdateExtent(axis, min, max);
}

//----------------------------------------------------------------------------
// Exports all the data from the input.
void vtkImageExport::Export(void *output)
{
  // Error checking
  if ( this->Input == NULL )
    {
    vtkErrorMacro(<<"Export: Please specify an input!");
    return;
    }
  
  // Fill in image information.
  this->Input->UpdateImageInformation();
  this->Input->SetUpdateExtent(this->Input->GetWholeExtent());
  this->UpdateProgress(0.0);
  this->RecursiveExport(2, this->Input, &output); 
}

//----------------------------------------------------------------------------
// Provides a valid pointer to the data (only valid until the next
// update, though)

void *vtkImageExport::GetPointerToData()
{
  // Error checking
  if ( this->Input == NULL )
    {
    vtkErrorMacro(<<"Export: Please specify an input!");
    return 0;
    }

  vtkImageCache *input = this->Input;

  // flip data if necessary
  if (this->ImageLowerLeft == 0)
    {
    if (this->ImageFlip == NULL)
      {
      this->ImageFlip = vtkImageFlip::New();
      this->ImageFlip->SetInput(input);
      this->ImageFlip->SetFilteredAxis(1);
      input = this->ImageFlip->GetOutput();
      }
    }
  else
    {
    if (this->ImageFlip)
      {
      this->ImageFlip->UnRegister(this);
      this->ImageFlip = NULL;
      }
    }

  if (this->GetDataMemorySize() > input->GetMemoryLimit())
    {
    input->SetMemoryLimit(this->GetDataMemorySize());
    }
  input->SetUpdateExtent(input->GetWholeExtent());
  input->ReleaseDataFlagOff();

  this->UpdateProgress(0.0);
  vtkImageData *data = input->UpdateAndReturnData();
  this->UpdateProgress(1.0);

  return data->GetScalarPointer();
}
  
  





