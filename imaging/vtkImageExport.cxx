/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageExport.cxx
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
#include <math.h>
#include "vtkImageExport.h"
#include "vtkBitmap.h"



//----------------------------------------------------------------------------
vtkImageExport::vtkImageExport()
{
  int idx;

  this->ScalarType = VTK_VOID;
  this->Input = NULL;
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Axes[idx] = idx;
    this->Extent[idx] = this->Extent[idx + VTK_IMAGE_DIMENSIONS] = 0;
    }
}



//----------------------------------------------------------------------------
vtkImageExport::~vtkImageExport()
{
}


//----------------------------------------------------------------------------
void vtkImageExport::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Input: (" << this->Input << ")\n";
  os << indent << "Axes: (" << this->Axes[0] << ", " << this->Axes[1] 
     << ", " << this->Axes[2] << ", " << this->Axes[3] << ", " 
     << this->Axes[4] << ")\n";
  os << indent << "Extent: (" << this->Extent[0] << ", " << this->Extent[1] 
     << ", " << this->Extent[2] << ", " << this->Extent[3] << ", " 
     << this->Extent[4] << ", " << this->Extent[5] << ")\n";
  os << indent << "ScalarType: " 
     << vtkImageScalarTypeNameMacro(this->ScalarType) << "\n";
}



//----------------------------------------------------------------------------
void vtkImageExport::SetAxes(int num, int *axes)
{
  vtkImageRegion *temp = vtkImageRegion::New();
  
  this->Modified();
  // Use a region to fill in the unspecified axes
  temp->SetAxes(num, axes);
  temp->GetAxes(VTK_IMAGE_DIMENSIONS, this->Axes);
  temp->Delete();
}
//----------------------------------------------------------------------------
void vtkImageExport::GetAxes(int num, int *axes)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "GetAxes: Requesting too many axes");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    axes[idx] = this->Axes[idx];
    }
  
}

  

//----------------------------------------------------------------------------
void vtkImageExport::SetExtent(int num, int *extent)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "SetExtent: " << num << "is to large.");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  this->Modified();
  for (idx = 0; idx < num*2; ++idx)
    {
    this->Extent[idx] = extent[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageExport::GetExtent(int num, int *extent)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "GetExtent: Requesting too large");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num*2; ++idx)
    {
    extent[idx] = this->Extent[idx];
    }
  
}

//----------------------------------------------------------------------------
void *vtkImageExport::GetPointer()
{
  int idx, length;
  unsigned char *array;
  
  // Compute the length of the memory (in bytes)
  switch (this->ScalarType)
    {
    case VTK_FLOAT:
      length = sizeof(float);
      break;
    case VTK_INT:
      length = sizeof(int);
      break;
    case VTK_SHORT:
      length = sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      length = sizeof(unsigned short);
      break;
    case VTK_UNSIGNED_CHAR:
      length = sizeof(unsigned char);
      break;
    default:
      vtkErrorMacro("Unknown ScalarType");
      return NULL;
    }
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    length *= this->Extent[idx*2+1] - this->Extent[idx*2] + 1;
    }
  
  array = new unsigned char[length];
  this->UpdateMemory(array);
  
  return (void *)array;
}

  
  

//----------------------------------------------------------------------------
void vtkImageExport::UpdateMemory(void *ptr)
{
  vtkImageRegion *region = vtkImageRegion::New();
  vtkImageData *data;
  int needToCopy;
  int *extent;
  int *axes;
  int length, idx;
  void *regionPtr;
  
  if ( ! this->Input)
    {
    vtkErrorMacro("No input");
    return;
    }
  
  region->SetScalarType(this->ScalarType);
  region->SetAxes(VTK_IMAGE_DIMENSIONS, this->Axes);
  // We have to tell the data to use the requested order.
  data = region->GetData();
  data->SetAxes(this->Axes);
  // Now we can set the extent of the region.
  region->SetExtent(VTK_IMAGE_DIMENSIONS, this->Extent);

  // Note: This may produce a copy warning.  We should disable
  // this warning in some way.
  this->Input->Update(region);
  
  // check to see if the cache gave us the data the way we wanted.
  needToCopy = 0;
  data = region->GetData(); // just in case the data has changed
  axes = data->GetAxes();
  extent = data->GetExtent();
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS && !needToCopy; ++idx)
    {
    if (axes[idx] != this->Axes[idx])
      {
      needToCopy = 1;
      vtkDebugMacro("Need to copy because of data order.");
      }
    if (extent[idx*2] != this->Extent[idx*2] ||
	extent[idx*2 + 1] != this->Extent[idx*2 + 1])
      {
      needToCopy = 1;
      vtkDebugMacro("Need to copy because axis " << idx << " extent.");
      }
    }
  if (! needToCopy && (this->ScalarType != data->GetScalarType()))
    {
    needToCopy = 1;
    vtkDebugMacro("Need to copy because of ScalarType.");
    }

  // Copy the data into the correct format if required.
  if (needToCopy)
    {
    vtkImageRegion *newRegion = vtkImageRegion::New();
    vtkImageData *newData = vtkImageData::New();

    newData->SetScalarType(this->ScalarType);
    newData->SetAxes(VTK_IMAGE_DIMENSIONS, this->Axes);
    newData->SetExtent(VTK_IMAGE_DIMENSIONS, this->Extent);
    
    newRegion->SetScalarType(this->ScalarType);
    newRegion->SetAxes(VTK_IMAGE_DIMENSIONS, this->Axes);
    newRegion->SetExtent(VTK_IMAGE_DIMENSIONS, this->Extent);
    newRegion->SetData(newData);

    // Copy:
    newRegion->CopyRegionData(region);
    region->Delete();
    region = newRegion;
    }
  
  // Compute the size of the memory (in bytes)
  switch (this->ScalarType)
    {
    case VTK_FLOAT:
      length = sizeof(float);
      break;
    case VTK_INT:
      length = sizeof(int);
      break;
    case VTK_SHORT:
      length = sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      length = sizeof(unsigned short);
      break;
    case VTK_UNSIGNED_CHAR:
      length = sizeof(unsigned char);
      break;
    default:
      vtkErrorMacro("Unknown ScalarType");
      region->Delete();
      return;
    }
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    length *= this->Extent[idx*2+1] - this->Extent[idx*2] + 1;
    }
  
  // Copy into the users array.
  regionPtr = region->GetScalarPointer();
  memcpy(ptr, regionPtr, length);
  region->Delete();
}









